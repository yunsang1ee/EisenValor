#include "stdafxClientFramework.h"
#include "SceneQueryWorld.h"
#include "Scene.h"
#include "MeshResource.h"
#include "TriangleSceneQuery.h"
#include "Transform.h"
#include <DirectXCollision.h>
#include <chrono>

void SceneQueryWorld::Clear()
{
	m_instances.clear();
	m_bvh.Clear();
	m_valid = false;
}

void SceneQueryWorld::Prepare(Scene& scene)
{
	const uint64_t revision = MeshComponent::GetGlobalRenderRevision();
	if (m_valid && m_revision == revision)
		return;
	const auto begin = std::chrono::steady_clock::now();
	Clear();
	std::vector<SceneQueryBounds> bounds;
	if (auto* storage = scene.GetStorage<MeshComponent>())
	{
		for (const auto& component : storage->GetList())
		{
			const auto* mesh = component.GetMeshResource();
			auto*		owner = component.GetGameObject();
			if (!mesh || !owner || component.GetQueryMask() == 0 || component.GetMobility() != RenderMobility::Static)
				continue;
			const auto* geometry = mesh->GetQueryGeometry();
			if (!geometry || geometry->Empty())
				continue;
			Instance instance;
			instance.component = component.GetHandle();
			instance.mesh = mesh;
			instance.queryMask = component.GetQueryMask();
			instance.world = owner->GetTransform().GetWorldMatrix();
			const auto		  world = DirectX::XMLoadFloat4x4(&instance.world);
			DirectX::XMVECTOR determinant;
			const auto		  inverse = DirectX::XMMatrixInverse(&determinant, world);
			if (std::abs(DirectX::XMVectorGetX(determinant)) < 1e-12f)
				continue;
			DirectX::XMStoreFloat4x4(&instance.inverseWorld, inverse);
			const auto&			 localBounds = mesh->GetBounds();
			DirectX::BoundingBox localBox, worldBox;
			DirectX::BoundingBox::CreateFromPoints(
				localBox,
				DirectX::XMVectorSet(localBounds.aabbmin[0], localBounds.aabbmin[1], localBounds.aabbmin[2], 0),
				DirectX::XMVectorSet(localBounds.aabbmax[0], localBounds.aabbmax[1], localBounds.aabbmax[2], 0)
			);
			localBox.Transform(worldBox, world);
			bounds.push_back(
				{{worldBox.Center.x - worldBox.Extents.x, worldBox.Center.y - worldBox.Extents.y,
				  worldBox.Center.z - worldBox.Extents.z},
				 {worldBox.Center.x + worldBox.Extents.x, worldBox.Center.y + worldBox.Extents.y,
				  worldBox.Center.z + worldBox.Extents.z}}
			);
			geometry->Prepare();
			m_instances.push_back(instance);
		}
	}
	m_bvh.Build(bounds);
	m_revision = revision;
	m_valid = true;
	DEBUG_LOG_FMT(
		"[SceneQuery] Static camera query instances={} build={:.2f}ms\n", m_instances.size(),
		std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count()
	);
}

bool SceneQueryWorld::SweepSphere(
	Scene&					 scene,
	const DirectX::XMFLOAT3& start,
	const DirectX::XMFLOAT3& end,
	float					 radius,
	uint32_t				 queryMask,
	float&					 fraction
)
{
	if (queryMask == 0)
		return false;
	Prepare(scene);
	auto* storage = scene.GetStorage<MeshComponent>();
	if (!storage)
		return false;
	const DirectX::XMFLOAT3 delta{end.x - start.x, end.y - start.y, end.z - start.z};
	const DirectX::XMFLOAT3 expansion{radius, radius, radius};
	bool					hit = false;
	m_bvh.VisitSegment(
		start, delta, expansion, fraction,
		[&](uint32_t index, float& limit)
		{
			const auto& instance = m_instances[index];
			if ((instance.queryMask & queryMask) == 0)
				return;
			const auto* component = storage->Get(instance.component);
			if (!component || !component->IsEnabled() || !component->IsOwnerActiveInHierarchy() ||
				component->GetMeshResource() != instance.mesh)
				return;
			hit |= instance.mesh->GetQueryGeometry()->SweepSphere(
				start, end, radius, instance.world, instance.inverseWorld, limit
			);
		}
	);
	return hit;
}
