#pragma once

#include <DirectXMath.h>
#include <span>

// CPU query geometry. Half extents are world-space lengths; orientation must be a unit quaternion.
struct SceneQueryBox
{
	DirectX::XMFLOAT3 center{};
	DirectX::XMFLOAT3 halfExtents{};
	DirectX::XMFLOAT4 orientation{0, 0, 0, 1};
};

struct BoxSceneQueryHit
{
	float			  fraction = 1.0f;
	DirectX::XMFLOAT3 normal{};
};

namespace BoxSceneQuery
{
// Segment raycast against an OBB expanded by clearance. Conservative at rounded corners:
// this protects a camera sphere without claiming an exact triangle/sphere sweep.
bool Cast(
	const DirectX::XMFLOAT3& start,
	const DirectX::XMFLOAT3& displacement,
	float					 clearance,
	const SceneQueryBox&	 box,
	BoxSceneQueryHit&		 hit
);
bool Overlaps(const DirectX::XMFLOAT3& position, float clearance, const SceneQueryBox& box);

// Resolve initial overlaps (e.g. a moving door), then sweep and slide along contact planes.
DirectX::XMFLOAT3 MoveAndSlide(
	const DirectX::XMFLOAT3&	   start,
	const DirectX::XMFLOAT3&	   displacement,
	float						   clearance,
	std::span<const SceneQueryBox> boxes,
	float						   skin = 0.01f
);
} // namespace BoxSceneQuery
