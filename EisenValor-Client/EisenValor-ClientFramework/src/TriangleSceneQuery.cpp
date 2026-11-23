#include "stdafxClientFramework.h"
#include "TriangleSceneQuery.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;
namespace
{
float Dot(FXMVECTOR a, FXMVECTOR b)
{
	return XMVectorGetX(XMVector3Dot(a, b));
}
bool Inside(FXMVECTOR p, FXMVECTOR a, FXMVECTOR b, GXMVECTOR c, HXMVECTOR normal)
{
	return Dot(XMVector3Cross(b - a, p - a), normal) >= -1e-6f && Dot(XMVector3Cross(c - b, p - b), normal) >= -1e-6f &&
		   Dot(XMVector3Cross(a - c, p - c), normal) >= -1e-6f;
}

// Earliest ray/sphere root, with the segment parameter kept in [0, limit].
bool Quadratic(float a, float b, float c, float limit, float& t)
{
	if (a < 1e-12f)
		return false;
	const float discriminant = b * b - a * c;
	if (discriminant < 0)
		return false;
	t = (-b - std::sqrt(discriminant)) / a;
	return t >= 0 && t <= limit;
}

bool SweepTriangle(FXMVECTOR start, FXMVECTOR delta, float radius, FXMVECTOR a, GXMVECTOR b, HXMVECTOR c, float& limit)
{
	const auto	cross = XMVector3Cross(b - a, c - a);
	const float normSq = Dot(cross, cross);
	if (normSq < 1e-14f)
		return false;
	const auto	normal = cross / std::sqrt(normSq);
	const float distance = Dot(start - a, normal), velocity = Dot(delta, normal);
	if (std::abs(distance) < radius && Inside(start - normal * distance, a, b, c, normal))
	{
		limit = 0;
		return true;
	}
	bool hit = false;
	if (std::abs(velocity) > 1e-8f)
	{
		for (float side : {-1.0f, 1.0f})
		{
			if (side * velocity >= 0)
				continue;
			const float t = (side * radius - distance) / velocity;
			if (t >= 0 && t <= limit && Inside(start + delta * t - normal * (side * radius), a, b, c, normal))
			{
				limit = t;
				hit = true;
			}
		}
	}
	const XMVECTOR points[] = {a, b, c};
	for (int i = 0; i < 3; ++i)
	{
		const auto	origin = start - points[i];
		const auto	edge = points[(i + 1) % 3] - points[i];
		const float edgeSq = Dot(edge, edge);
		if (edgeSq < 1e-14f)
			continue;
		const float projection = Dot(origin, edge) / edgeSq;
		const auto	separation = origin - edge * std::clamp(projection, 0.0f, 1.0f);
		if (Dot(separation, separation) < radius * radius)
		{
			limit = 0;
			return true;
		}
		float t = 0;
		if (Quadratic(Dot(delta, delta), Dot(origin, delta), Dot(origin, origin) - radius * radius, limit, t))
		{
			limit = t;
			hit = true;
		}
		const float speed = Dot(delta, edge) / edgeSq;
		const auto	perpendicularOrigin = origin - edge * projection;
		const auto	perpendicularDelta = delta - edge * speed;
		if (Quadratic(
				Dot(perpendicularDelta, perpendicularDelta), Dot(perpendicularOrigin, perpendicularDelta),
				Dot(perpendicularOrigin, perpendicularOrigin) - radius * radius, limit, t
			))
		{
			const float edgePosition = projection + speed * t;
			if (edgePosition >= 0 && edgePosition <= 1)
			{
				limit = t;
				hit = true;
			}
		}
	}
	return hit;
}
} // namespace

void TriangleSceneQuery::SetGeometry(std::vector<XMFLOAT3> vertices, std::vector<uint32_t> indices)
{
	m_vertices = std::move(vertices);
	m_indices.clear();
	m_indices.reserve(indices.size());
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		if (indices[i] < m_vertices.size() && indices[i + 1] < m_vertices.size() && indices[i + 2] < m_vertices.size())
			m_indices.insert(m_indices.end(), {indices[i], indices[i + 1], indices[i + 2]});
	}
	m_bvh.Clear();
	m_prepared = false;
}

void TriangleSceneQuery::Prepare() const
{
	if (m_prepared)
		return;
	std::vector<SceneQueryBounds> boxes;
	boxes.reserve(m_indices.size() / 3);
	for (size_t i = 0; i < m_indices.size(); i += 3)
	{
		const auto& a = m_vertices[m_indices[i]];
		const auto& b = m_vertices[m_indices[i + 1]];
		const auto& c = m_vertices[m_indices[i + 2]];
		boxes.push_back(
			{{std::min({a.x, b.x, c.x}), std::min({a.y, b.y, c.y}), std::min({a.z, b.z, c.z})},
			 {std::max({a.x, b.x, c.x}), std::max({a.y, b.y, c.y}), std::max({a.z, b.z, c.z})}}
		);
	}
	m_bvh.Build(boxes);
	m_prepared = true;
}

bool TriangleSceneQuery::SweepSphere(
	const XMFLOAT3&	  start,
	const XMFLOAT3&	  end,
	float			  radius,
	const XMFLOAT4X4& world,
	const XMFLOAT4X4& inverseWorld,
	float&			  fraction
) const
{
	Prepare();
	const auto inverse = XMLoadFloat4x4(&inverseWorld), matrix = XMLoadFloat4x4(&world);
	const auto begin = XMLoadFloat3(&start), delta = XMLoadFloat3(&end) - begin;
	XMFLOAT3   localStart, localDelta;
	XMStoreFloat3(&localStart, XMVector3TransformCoord(begin, inverse));
	XMStoreFloat3(&localDelta, XMVector3TransformNormal(delta, inverse));
	// The inverse image of a world sphere is an ellipsoid. Its axis extents conservatively bound the local BVH query.
	const XMFLOAT3 expansion = {
		radius * std::sqrt(
					 inverseWorld._11 * inverseWorld._11 + inverseWorld._21 * inverseWorld._21 +
					 inverseWorld._31 * inverseWorld._31
				 ),
		radius * std::sqrt(
					 inverseWorld._12 * inverseWorld._12 + inverseWorld._22 * inverseWorld._22 +
					 inverseWorld._32 * inverseWorld._32
				 ),
		radius * std::sqrt(
					 inverseWorld._13 * inverseWorld._13 + inverseWorld._23 * inverseWorld._23 +
					 inverseWorld._33 * inverseWorld._33
				 )
	};
	bool hit = false;
	m_bvh.VisitSegment(
		localStart, localDelta, expansion, fraction,
		[&](uint32_t triangle, float& limit)
		{
			const auto offset = static_cast<size_t>(triangle) * 3;
			const auto a = XMVector3TransformCoord(XMLoadFloat3(&m_vertices[m_indices[offset]]), matrix);
			const auto b = XMVector3TransformCoord(XMLoadFloat3(&m_vertices[m_indices[offset + 1]]), matrix);
			const auto c = XMVector3TransformCoord(XMLoadFloat3(&m_vertices[m_indices[offset + 2]]), matrix);
			hit |= SweepTriangle(begin, delta, radius, a, b, c, limit);
		}
	);
	return hit;
}
