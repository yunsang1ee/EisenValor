#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

struct SceneQueryBounds
{
	DirectX::XMFLOAT3 minimum;
	DirectX::XMFLOAT3 maximum;
};

// Balanced CPU BVH shared by instance and local-triangle query levels.
class SceneQueryBvh
{
public:
	void Clear()
	{
		m_nodes.clear();
		m_order.clear();
	}
	void Build(std::span<const SceneQueryBounds> bounds)
	{
		Clear();
		m_order.resize(bounds.size());
		std::iota(m_order.begin(), m_order.end(), 0u);
		if (bounds.empty())
			return;
		m_nodes.reserve(bounds.size() / 2 + 1);
		m_nodes.resize(1);
		BuildNode(0, 0, static_cast<uint32_t>(bounds.size()), bounds);
	}

	template <typename Visitor>
	void VisitSegment(
		const DirectX::XMFLOAT3& origin,
		const DirectX::XMFLOAT3& delta,
		const DirectX::XMFLOAT3& expansion,
		float&					 maxFraction,
		Visitor&&				 visitor
	) const
	{
		if (m_nodes.empty())
			return;
		// Median splitting bounds depth below 32 for a uint32_t primitive count.
		std::array<uint32_t, 64> stack{};
		size_t					 count = 1;
		while (count != 0)
		{
			const auto& node = m_nodes[stack[--count]];
			if (!Intersects(node.bounds, origin, delta, expansion, maxFraction))
				continue;
			if (node.count != 0)
			{
				for (uint32_t i = 0; i < node.count; ++i)
					visitor(m_order[node.first + i], maxFraction);
			}
			else
			{
				stack[count++] = node.first;
				stack[count++] = node.first + 1;
			}
		}
	}

private:
	struct Node
	{
		SceneQueryBounds bounds;
		uint32_t		 first = 0;
		uint32_t		 count = 0;
	};
	std::vector<Node>	  m_nodes;
	std::vector<uint32_t> m_order;
	static float		  Axis(const DirectX::XMFLOAT3& value, int axis)
	{
		return axis == 0 ? value.x : axis == 1 ? value.y : value.z;
	}
	static bool Intersects(
		const SceneQueryBounds&	 box,
		const DirectX::XMFLOAT3& origin,
		const DirectX::XMFLOAT3& delta,
		const DirectX::XMFLOAT3& expansion,
		float					 maxFraction
	)
	{
		float enter = 0, exit = maxFraction;
		for (int axis = 0; axis < 3; ++axis)
		{
			const float o = Axis(origin, axis), d = Axis(delta, axis);
			const float lo = Axis(box.minimum, axis) - Axis(expansion, axis);
			const float hi = Axis(box.maximum, axis) + Axis(expansion, axis);
			if (std::abs(d) < 1e-8f)
			{
				if (o < lo || o > hi)
					return false;
				continue;
			}
			float a = (lo - o) / d, b = (hi - o) / d;
			if (a > b)
				std::swap(a, b);
			enter = std::max(enter, a);
			exit = std::min(exit, b);
			if (enter > exit)
				return false;
		}
		return true;
	}
	void BuildNode(uint32_t nodeIndex, uint32_t first, uint32_t count, std::span<const SceneQueryBounds> boxes)
	{
		auto bounds = boxes[m_order[first]];
		for (uint32_t i = 1; i < count; ++i)
		{
			const auto& next = boxes[m_order[first + i]];
			bounds.minimum = {
				std::min(bounds.minimum.x, next.minimum.x), std::min(bounds.minimum.y, next.minimum.y),
				std::min(bounds.minimum.z, next.minimum.z)
			};
			bounds.maximum = {
				std::max(bounds.maximum.x, next.maximum.x), std::max(bounds.maximum.y, next.maximum.y),
				std::max(bounds.maximum.z, next.maximum.z)
			};
		}
		m_nodes[nodeIndex].bounds = bounds;
		if (count <= 8)
		{
			m_nodes[nodeIndex].first = first;
			m_nodes[nodeIndex].count = count;
			return;
		}
		const float sizes[] = {
			bounds.maximum.x - bounds.minimum.x, bounds.maximum.y - bounds.minimum.y,
			bounds.maximum.z - bounds.minimum.z
		};
		const int	   axis = sizes[1] > sizes[0] ? (sizes[2] > sizes[1] ? 2 : 1) : (sizes[2] > sizes[0] ? 2 : 0);
		const uint32_t middle = first + count / 2;
		std::nth_element(
			m_order.begin() + first, m_order.begin() + middle, m_order.begin() + first + count,
			[&](uint32_t a, uint32_t b)
			{
				return Axis(boxes[a].minimum, axis) + Axis(boxes[a].maximum, axis) <
					   Axis(boxes[b].minimum, axis) + Axis(boxes[b].maximum, axis);
			}
		);
		const auto child = static_cast<uint32_t>(m_nodes.size());
		m_nodes[nodeIndex].first = child;
		m_nodes.resize(m_nodes.size() + 2);
		BuildNode(child, first, count / 2, boxes);
		BuildNode(child + 1, middle, count - count / 2, boxes);
	}
};
