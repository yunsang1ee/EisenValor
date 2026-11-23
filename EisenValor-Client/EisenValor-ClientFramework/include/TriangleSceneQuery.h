#pragma once
#include "SceneQueryBvh.h"

class TriangleSceneQuery
{
public:
	void SetGeometry(std::vector<DirectX::XMFLOAT3> vertices, std::vector<uint32_t> indices);
	void Prepare() const;
	bool SweepSphere(
		const DirectX::XMFLOAT3&   start,
		const DirectX::XMFLOAT3&   end,
		float					   radius,
		const DirectX::XMFLOAT4X4& world,
		const DirectX::XMFLOAT4X4& inverseWorld,
		float&					   fraction
	) const;
	bool Empty() const { return m_indices.empty(); }

private:
	std::vector<DirectX::XMFLOAT3> m_vertices;
	std::vector<uint32_t>		   m_indices;
	mutable SceneQueryBvh		   m_bvh;
	mutable bool				   m_prepared = false;
};
