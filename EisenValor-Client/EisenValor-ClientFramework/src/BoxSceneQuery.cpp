#include "stdafxClientFramework.h"
#include "BoxSceneQuery.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
using namespace DirectX;
constexpr float kMotionEpsilon = 1.0e-6f;

XMFLOAT3 LocalPosition(const XMFLOAT3& position, const SceneQueryBox& box)
{
	XMFLOAT3 result;
	XMStoreFloat3(
		&result,
		XMVector3InverseRotate(XMLoadFloat3(&position) - XMLoadFloat3(&box.center), XMLoadFloat4(&box.orientation))
	);
	return result;
}

XMFLOAT3 WorldNormal(int axis, float sign, const SceneQueryBox& box)
{
	XMFLOAT3 normal{};
	if (axis == 0)
		normal.x = sign;
	if (axis == 1)
		normal.y = sign;
	if (axis == 2)
		normal.z = sign;
	XMStoreFloat3(&normal, XMVector3Rotate(XMLoadFloat3(&normal), XMLoadFloat4(&box.orientation)));
	return normal;
}

bool GetPenetration(const XMFLOAT3& position, float clearance, const SceneQueryBox& box, float& depth, XMFLOAT3& normal)
{
	const auto	local = LocalPosition(position, box);
	const float coordinates[] = {local.x, local.y, local.z};
	const float extents[] = {
		box.halfExtents.x + clearance, box.halfExtents.y + clearance, box.halfExtents.z + clearance
	};
	depth = std::numeric_limits<float>::max();
	int face = 0;
	for (int axis = 0; axis < 3; ++axis)
	{
		const float overlap = extents[axis] - std::abs(coordinates[axis]);
		if (overlap <= kMotionEpsilon)
			return false;
		if (overlap < depth)
		{
			depth = overlap;
			face = axis;
		}
	}
	normal = WorldNormal(face, coordinates[face] >= 0 ? 1.0f : -1.0f, box);
	return true;
}

XMFLOAT3 Depenetrate(XMFLOAT3 position, float clearance, std::span<const SceneQueryBox> boxes, float skin)
{
	for (int iteration = 0; iteration < 12; ++iteration)
	{
		float	 deepest = 0;
		XMFLOAT3 correctionNormal{};
		for (const auto& box : boxes)
		{
			float	 depth = 0;
			XMFLOAT3 normal{};
			if (GetPenetration(position, clearance, box, depth, normal) && depth > deepest)
			{
				deepest = depth;
				correctionNormal = normal;
			}
		}
		if (deepest == 0)
			break;
		XMStoreFloat3(&position, XMLoadFloat3(&position) + XMLoadFloat3(&correctionNormal) * (deepest + skin));
	}
	return position;
}
} // namespace

bool BoxSceneQuery::Cast(
	const XMFLOAT3&		 start,
	const XMFLOAT3&		 displacement,
	float				 clearance,
	const SceneQueryBox& box,
	BoxSceneQueryHit&	 hit
)
{
	const auto localStart = LocalPosition(start, box);
	XMFLOAT3   localDelta;
	XMStoreFloat3(&localDelta, XMVector3InverseRotate(XMLoadFloat3(&displacement), XMLoadFloat4(&box.orientation)));
	const float origin[] = {localStart.x, localStart.y, localStart.z};
	const float delta[] = {localDelta.x, localDelta.y, localDelta.z};
	const float extents[] = {
		box.halfExtents.x + clearance, box.halfExtents.y + clearance, box.halfExtents.z + clearance
	};
	float enter = 0;
	float exit = 1;
	int	  face = -1;
	float faceSign = 1;
	for (int axis = 0; axis < 3; ++axis)
	{
		if (std::abs(delta[axis]) < kMotionEpsilon)
		{
			if (std::abs(origin[axis]) > extents[axis])
				return false;
			continue;
		}
		float		nearTime = (-extents[axis] - origin[axis]) / delta[axis];
		float		farTime = (extents[axis] - origin[axis]) / delta[axis];
		const float sign = delta[axis] > 0 ? -1.0f : 1.0f;
		if (nearTime > farTime)
			std::swap(nearTime, farTime);
		if (nearTime >= enter)
		{
			enter = nearTime;
			face = axis;
			faceSign = sign;
		}
		exit = std::min(exit, farTime);
		if (enter > exit)
			return false;
	}
	if (face < 0 || enter < 0 || enter > 1 || exit < 0)
		return false;
	hit.fraction = enter;
	hit.normal = WorldNormal(face, faceSign, box);
	return true;
}

bool BoxSceneQuery::Overlaps(const XMFLOAT3& position, float clearance, const SceneQueryBox& box)
{
	float	 depth;
	XMFLOAT3 normal;
	return GetPenetration(position, clearance, box, depth, normal);
}

XMFLOAT3 BoxSceneQuery::MoveAndSlide(
	const XMFLOAT3&				   start,
	const XMFLOAT3&				   displacement,
	float						   clearance,
	std::span<const SceneQueryBox> boxes,
	float						   skin
)
{
	clearance = std::max(clearance, 0.0f);
	skin = std::max(skin, 0.001f);
	XMFLOAT3 position = Depenetrate(start, clearance, boxes, skin);
	XMFLOAT3 remaining = displacement;
	for (int iteration = 0; iteration < 5; ++iteration)
	{
		const float length = XMVectorGetX(XMVector3Length(XMLoadFloat3(&remaining)));
		if (length < kMotionEpsilon)
			break;
		BoxSceneQueryHit closest;
		bool			 collided = false;
		for (const auto& box : boxes)
		{
			BoxSceneQueryHit candidate;
			if (Cast(position, remaining, clearance, box, candidate) && candidate.fraction <= closest.fraction)
			{
				closest = candidate;
				collided = true;
			}
		}
		if (!collided)
		{
			XMStoreFloat3(&position, XMLoadFloat3(&position) + XMLoadFloat3(&remaining));
			break;
		}
		const float fraction = std::max(0.0f, closest.fraction - skin / length);
		XMStoreFloat3(&position, XMLoadFloat3(&position) + XMLoadFloat3(&remaining) * fraction);
		auto		slide = XMLoadFloat3(&remaining) * (1.0f - fraction);
		const auto	normal = XMLoadFloat3(&closest.normal);
		const float intoSurface = XMVectorGetX(XMVector3Dot(slide, normal));
		if (intoSurface < 0)
			slide -= normal * intoSurface;
		XMStoreFloat3(&remaining, slide);
	}
	return Depenetrate(position, clearance, boxes, skin);
}
