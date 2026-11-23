#pragma once
#include "SceneQueryBvh.h"
#include "MeshComponent.h"

class Scene;
class MeshResource;

// Opt-in static render-mesh fallback. Query-mask meaning is supplied by the game.
class SceneQueryWorld
{
public:
	void Clear();
	void Prepare(Scene& scene);
	bool SweepSphere(
		Scene&					 scene,
		const DirectX::XMFLOAT3& start,
		const DirectX::XMFLOAT3& end,
		float					 radius,
		uint32_t				 queryMask,
		float&					 fraction
	);

private:
	struct Instance
	{
		HandleOf<MeshComponent> component;
		const MeshResource*		mesh = nullptr;
		DirectX::XMFLOAT4X4		world;
		DirectX::XMFLOAT4X4		inverseWorld;
		uint32_t				queryMask = 0;
	};
	std::vector<Instance> m_instances;
	SceneQueryBvh		  m_bvh;
	uint64_t			  m_revision = 0;
	bool				  m_valid = false;
};
