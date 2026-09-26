#include "stdafxClient.h"
#include "RaytracingPreparePass.h"
#include "Scene.h"
#include "DxFrameResource.h"
#include "DxCommandContext.h"
#include "PixProfiler.h"
#include "DxDeviceGlobal.h"
#include "ResourceGlobal.h"
#include "MeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "DxBLAS.h"
#include "DxBuffer.h"
#include "Commonutils.h"
#include "MeshResource.h"
#include "MaterialResource.h"
#include "TextureResource.h"
#include "RaytracingCommon.h"
#include "RenderContext.h"
#include "CameraRenderData.h"
#include "FrameRenderData.h"
#include "DxDescriptorHeapGlobal.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

namespace
{
constexpr uint32_t kStableAssetMaterialIdSalt = 0x4d415441u;		  // MATA
constexpr uint32_t kStableRuntimeMaterialIdSalt = 0x4d415452u;		  // MATR
constexpr uint32_t kStableStaticAssetGeometryIdSalt = 0x47454f53u;	  // GEOS
constexpr uint32_t kStableSkinnedAssetGeometryIdSalt = 0x47454f4bu;	  // GEOK
constexpr uint32_t kStableStaticRuntimeGeometryIdSalt = 0x47455253u;  // GERS
constexpr uint32_t kStableSkinnedRuntimeGeometryIdSalt = 0x4745524bu; // GERK

constexpr float kInstanceMotionTeleportDistance = 10.0f;
constexpr float kInstanceMotionTeleportDistanceSq = kInstanceMotionTeleportDistance * kInstanceMotionTeleportDistance;

void HashTlasInstance(uint64_t& topologyHash, uint64_t& transformHash, const DxTLASInstance& instance)
{
	auto*		   obj = instance.obj;
	const auto*	   blas = instance.blas;
	const auto	   handle = obj ? obj->GetHandle() : GameObject::Handle::Invalid();
	const uint64_t blasAddress = blas ? blas->GetGPUAddress() : 0ull;
	const uint32_t flags = static_cast<uint32_t>(instance.flags);

	Utils::AppendFnv1a64(topologyHash, &handle.id, sizeof(handle.id));
	Utils::AppendFnv1a64(topologyHash, &handle.generation, sizeof(handle.generation));
	Utils::AppendFnv1a64(topologyHash, &blasAddress, sizeof(blasAddress));
	Utils::AppendFnv1a64(topologyHash, &flags, sizeof(flags));

	if (nullptr != obj)
	{
		const auto worldMatrix = obj->GetWorldMatrix();
		Utils::AppendFnv1a64(transformHash, &worldMatrix, sizeof(worldMatrix));
	}
}

uint32_t MakeStableAssetId(uint32_t salt, const EvAsset::Guid& guid, uint32_t localIndex = ~0u)
{
	uint64_t hash = Utils::kFnv1a64OffsetBasis;
	Utils::AppendFnv1a64(hash, &salt, sizeof(salt));
	Utils::AppendFnv1a64(hash, guid.data, sizeof(guid.data));
	if (localIndex != ~0u)
	{
		Utils::AppendFnv1a64(hash, &localIndex, sizeof(localIndex));
	}

	uint32_t id = static_cast<uint32_t>(hash) ^ static_cast<uint32_t>(hash >> 32);
	return id == ~0u ? ~1u : id;
}

uint32_t MakeStableRuntimeId(uint32_t salt, uint32_t ownerId, uint32_t localIndex)
{
	uint64_t hash = Utils::kFnv1a64OffsetBasis;
	Utils::AppendFnv1a64(hash, &salt, sizeof(salt));
	Utils::AppendFnv1a64(hash, &ownerId, sizeof(ownerId));
	Utils::AppendFnv1a64(hash, &localIndex, sizeof(localIndex));
	uint32_t id = static_cast<uint32_t>(hash) ^ static_cast<uint32_t>(hash >> 32);
	return id == ~0u ? ~1u : id;
}

uint64_t MakeInstanceMotionKey(uint32_t ownerId, uint32_t generation)
{
	return static_cast<uint64_t>(ownerId) | (static_cast<uint64_t>(generation) << 32u);
}

bool IsContinuousInstanceMotion(const DirectX::XMFLOAT4X4& previousWorld, const DirectX::XMFLOAT4X4& currentWorld)
{
	const float deltaX = currentWorld._41 - previousWorld._41;
	const float deltaY = currentWorld._42 - previousWorld._42;
	const float deltaZ = currentWorld._43 - previousWorld._43;
	return deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ <= kInstanceMotionTeleportDistanceSq;
}

void InitializeStaticInstanceMotion(InstanceData& instance)
{
	instance.previousWorldMatrix = instance.worldMatrix;
	instance.previousVertexBufferIdx = instance.vertexBufferIdx;
	instance.motionFlags = INSTANCE_MOTION_HAS_PREVIOUS;
	instance.motionPad0 = 0u;
}

bool IsRestirEmissiveLightMaterial(const MaterialResource* material)
{
	if (nullptr == material)
	{
		return false;
	}

	const auto	emissive = material->GetEmissive();
	const float emissionMax = std::max(emissive.x, std::max(emissive.y, emissive.z)) * std::max(emissive.w, 0.0f);
	return emissionMax > 0.0f || 0 != (material->GetMaterialFlags() & MATERIAL_FLAG_EMISSIVE_MAP);
}

float EstimateRestirEmissiveLightSelectionWeight(const MaterialResource* material, uint32_t triangleCount)
{
	if (nullptr == material || 0 == triangleCount)
	{
		return 0.0f;
	}

	const auto	emissive = material->GetEmissive();
	const float emissionLuminance = (0.2126f * std::max(emissive.x, 0.0f) + 0.7152f * std::max(emissive.y, 0.0f) +
									 0.0722f * std::max(emissive.z, 0.0f)) *
									std::max(emissive.w, 0.0f);
	const bool	hasEmissiveMap = 0 != (material->GetMaterialFlags() & MATERIAL_FLAG_EMISSIVE_MAP);
	const float resolvedLuminance = hasEmissiveMap ? std::max(emissionLuminance, 1.0f) : emissionLuminance;
	return std::max(resolvedLuminance * static_cast<float>(triangleCount), 0.0001f);
}

float AppendRestirEmissiveLightCdf(float& weightSum, float selectionWeight)
{
	const float previousWeightSum = weightSum;
	float		cumulativeWeight = previousWeightSum + std::max(selectionWeight, 0.0f);
	if (selectionWeight > 0.0f && cumulativeWeight <= previousWeightSum)
	{
		cumulativeWeight = std::nextafter(previousWeightSum, std::numeric_limits<float>::infinity());
	}
	weightSum = cumulativeWeight;
	return cumulativeWeight;
}

template <typename T>
void AppendRestirHistoryTableHash(uint64_t& hash, std::span<const T> values)
{
	const uint64_t elementCount = static_cast<uint64_t>(values.size());
	Utils::AppendFnv1a64(hash, &elementCount, sizeof(elementCount));
	if (!values.empty())
	{
		Utils::AppendFnv1a64(hash, values.data(), values.size_bytes());
	}
}

void AppendRestirHistoryInstanceHash(uint64_t& hash, std::span<const DxTLASInstance> instances)
{
	const uint64_t instanceCount = static_cast<uint64_t>(instances.size());
	Utils::AppendFnv1a64(hash, &instanceCount, sizeof(instanceCount));
	for (const auto& instance : instances)
	{
		const auto	   handle = instance.obj ? instance.obj->GetHandle() : GameObject::Handle::Invalid();
		const uint32_t flags = static_cast<uint32_t>(instance.flags);
		Utils::AppendFnv1a64(hash, &handle.id, sizeof(handle.id));
		Utils::AppendFnv1a64(hash, &handle.generation, sizeof(handle.generation));
		Utils::AppendFnv1a64(hash, &flags, sizeof(flags));
	}
}

template <typename T>
std::span<const T> GetRestirHistoryDynamicSuffix(std::span<const T> values, size_t staticElementCount)
{
	assert(staticElementCount <= values.size() && "[ReSTIR] Static history prefix exceeds current frame table");
	return values.subspan(std::min(staticElementCount, values.size()));
}

uint64_t BuildStaticRestirHistorySignature(
	std::span<const DxTLASInstance>			 staticInstances,
	uint64_t								 staticSceneVersion,
	std::span<const MaterialGPUData>		 staticMaterials,
	std::span<const TerrainSurfaceGPUData>	 staticTerrainSurfaces,
	std::span<const GeoInfo>				 staticGeometry,
	std::span<const RestirEmissiveLightData> staticEmissiveLights
)
{
	PixScopedCpuEvent signatureEvent(L"DXR.BuildStaticRestirHistorySignature");
	uint64_t		  hash = Utils::kFnv1a64OffsetBasis;
	AppendRestirHistoryInstanceHash(hash, staticInstances);
	Utils::AppendFnv1a64(hash, &staticSceneVersion, sizeof(staticSceneVersion));
	AppendRestirHistoryTableHash(hash, staticMaterials);
	AppendRestirHistoryTableHash(hash, staticTerrainSurfaces);
	AppendRestirHistoryTableHash(hash, staticGeometry);
	AppendRestirHistoryTableHash(hash, staticEmissiveLights);
	return hash;
}

uint64_t BuildRestirHistorySignature(
	std::span<const DxTLASInstance> instances,
	const StaticSceneRenderData&	staticSceneData,
	const MaterialRenderData&		materialData,
	const GeoTableRenderData&		geoTableData,
	const RestirLightRenderData&	lightData,
	uint64_t						historyGeneration
)
{
	PixScopedCpuEvent signatureEvent(L"DXR.BuildRestirHistorySignature");
	uint64_t		  hash = Utils::kFnv1a64OffsetBasis;
	{
		PixScopedCpuEvent staticSeedEvent(L"DXR.BuildRestirHistorySignature.StaticSeed");
		Utils::AppendFnv1a64(
			hash, &staticSceneData.restirHistorySignatureSeed, sizeof(staticSceneData.restirHistorySignatureSeed)
		);
		Utils::AppendFnv1a64(hash, &historyGeneration, sizeof(historyGeneration));
	}
	{
		PixScopedCpuEvent instancesEvent(L"DXR.BuildRestirHistorySignature.DynamicInstances");
		AppendRestirHistoryInstanceHash(
			hash, GetRestirHistoryDynamicSuffix(instances, staticSceneData.tlasInstances.size())
		);
	}
	{
		PixScopedCpuEvent materialsEvent(L"DXR.BuildRestirHistorySignature.DynamicMaterials");
		AppendRestirHistoryTableHash(
			hash, GetRestirHistoryDynamicSuffix(materialData.syncBuffer.GetSpan(), staticSceneData.materials.size())
		);
	}
	{
		PixScopedCpuEvent terrainEvent(L"DXR.BuildRestirHistorySignature.DynamicTerrainSurfaces");
		AppendRestirHistoryTableHash(
			hash, GetRestirHistoryDynamicSuffix(
					  materialData.terrainSurfaceSyncBuffer.GetSpan(), staticSceneData.terrainSurfaces.size()
				  )
		);
	}
	{
		PixScopedCpuEvent geometryEvent(L"DXR.BuildRestirHistorySignature.DynamicGeometry");
		AppendRestirHistoryTableHash(
			hash, GetRestirHistoryDynamicSuffix(geoTableData.syncBuffer.GetSpan(), staticSceneData.geometry.size())
		);
	}
	{
		PixScopedCpuEvent emissiveEvent(L"DXR.BuildRestirHistorySignature.DynamicEmissiveLights");
		AppendRestirHistoryTableHash(
			hash,
			GetRestirHistoryDynamicSuffix(lightData.emissiveLightSync.GetSpan(), staticSceneData.emissiveLights.size())
		);
	}
	return hash;
}

uint32_t GetReadyTextureIndex(MaterialResource* material, std::string_view slotName)
{
	if (nullptr == material)
	{
		return ~0u;
	}

	auto texRes = material->GetTexture(slotName);
	if (!texRes || !texRes->IsReady() || nullptr == texRes->GetTexture())
	{
		return ~0u;
	}

	return texRes->GetTexture()->GetSRVIndex();
}


InstanceData MakeInstanceData(
	GameObject* object, uint32_t vertexBufferIndex, uint32_t indexBufferIndex, uint32_t geometryBase
)
{
	InstanceData instance = {};
	const auto	 worldFloat = object->GetTransform().GetWorldMatrix();
	const auto	 world = DirectX::XMLoadFloat4x4(&worldFloat);
	DirectX::XMStoreFloat4x4(&instance.worldMatrix, world);
	DirectX::XMStoreFloat4x4(&instance.worldInverse, DirectX::XMMatrixInverse(nullptr, world));
	instance.vertexBufferIdx = vertexBufferIndex;
	instance.indexBufferIdx = indexBufferIndex;
	instance.geoInfoBaseIdx = geometryBase;
	instance.instanceID = object->GetHandle().id;
	instance.generation = object->GetHandle().generation;
	return instance;
}

} // namespace

void RaytracingPreparePass::RegisterInstanceIdLookup(uint32_t ownerId, uint32_t instanceIndex)
{
	constexpr uint32_t kInvalidInstanceIndex = 0xffffffffu;
	if (ownerId == kInvalidInstanceIndex)
	{
		return;
	}

	const size_t requiredSize = static_cast<size_t>(ownerId) + 1u;
	if (requiredSize > m_instanceIdLookupScratch.capacity())
	{
		const size_t currentCapacity = m_instanceIdLookupScratch.capacity();
		const size_t grownCapacity = currentCapacity + (currentCapacity + 1u) / 2u;
		m_instanceIdLookupScratch.reserve(std::max(requiredSize, grownCapacity));
	}
	if (requiredSize > m_instanceIdLookupScratch.size())
	{
		m_instanceIdLookupScratch.resize(requiredSize, kInvalidInstanceIndex);
	}
	m_instanceIdLookupScratch[ownerId] = instanceIndex;
}

void RaytracingPreparePass::BeginInstanceMotionFrame(Scene* scene)
{
	if (m_instanceMotionScene != scene)
	{
		m_previousInstanceWorldMatrices.clear();
		m_hasPreviousInstanceFrame = false;
		m_instanceMotionScene = scene;
	}

	m_currentInstanceWorldMatrices.clear();
}

void RaytracingPreparePass::ApplyInstanceMotionHistory(
	InstanceData& instance, bool usePreviousSkinnedVertices, uint32_t previousVertexBufferIndex
)
{
	instance.previousWorldMatrix = instance.worldMatrix;
	instance.previousVertexBufferIdx = instance.vertexBufferIdx;
	instance.motionFlags = 0u;
	instance.motionPad0 = 0u;

	const uint64_t motionKey = MakeInstanceMotionKey(instance.instanceID, instance.generation);
	const auto	   previousWorld = m_previousInstanceWorldMatrices.find(motionKey);
	const bool	   hasPreviousGeometry = !usePreviousSkinnedVertices || previousVertexBufferIndex != 0xffffffffu;
	if (m_hasPreviousInstanceFrame && previousWorld != m_previousInstanceWorldMatrices.end() && hasPreviousGeometry &&
		IsContinuousInstanceMotion(previousWorld->second, instance.worldMatrix))
	{
		instance.previousWorldMatrix = previousWorld->second;
		instance.previousVertexBufferIdx =
			usePreviousSkinnedVertices ? previousVertexBufferIndex : instance.vertexBufferIdx;
		instance.motionFlags = INSTANCE_MOTION_HAS_PREVIOUS;
		if (usePreviousSkinnedVertices)
		{
			instance.motionFlags |= INSTANCE_MOTION_PREVIOUS_SKINNED;
		}
	}

	m_currentInstanceWorldMatrices.insert_or_assign(motionKey, instance.worldMatrix);
}

void RaytracingPreparePass::CommitInstanceMotionFrame(uint32_t frameIndex)
{
	m_previousInstanceWorldMatrices.swap(m_currentInstanceWorldMatrices);
	m_currentInstanceWorldMatrices.clear();
	m_previousInstanceFrameIndex = frameIndex;
	m_hasPreviousInstanceFrame = true;
}

void RaytracingPreparePass::PrepareRenderData(DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition)
{
	PixScopedCpuEvent cpuEvent(L"DXR.PrepareRenderData");
	auto&			  context = *frame->GetMainContext();

	const uint32_t frameIndex = frame->GetFrameIndex();
	auto*		   instanceData = &m_instanceData[frameIndex];
	auto*		   materialData = &m_materialData[frameIndex];
	auto*		   geoTableData = &m_geoTableData[frameIndex];
	auto*		   restirLightData = &m_restirLightData[frameIndex];
	auto&		   tlasFrame = m_tlas[frameIndex];
	auto*		   tlas = tlasFrame.Get();
	auto&		   staticSceneData = m_staticSceneData.Get();
	bool		   hasAnimatedInstances = false;
	uint32_t	   animatedBlasCount = 0;

	auto*							   cmdList = context.CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	const bool pendingLoadsActive = GLOBAL(ResourceGlobal).HasPendingLoads();
	auto*	   meshStorage = scene->GetStorage<MeshComponent>();
	size_t	   staticMeshComponentCount = 0;
	if (nullptr != meshStorage)
	{
		for (const auto& meshComp : meshStorage->GetList())
		{
			staticMeshComponentCount += meshComp.GetMobility() == RenderMobility::Static ? 1u : 0u;
		}
	}

	const uint64_t meshRevision = MeshComponent::GetGlobalRenderRevision();
	const bool	   pendingLoadsCompleted = staticSceneData.pendingLoadsActive && !pendingLoadsActive;
	const bool incompleteCacheReadyToRetry = staticSceneData.valid && !staticSceneData.complete && !pendingLoadsActive;
	bool	   rebuildStaticCache = !staticSceneData.valid || staticSceneData.scene != scene ||
							  staticSceneData.meshRevision != meshRevision ||
							  staticSceneData.meshComponentCount != staticMeshComponentCount || pendingLoadsCompleted ||
							  incompleteCacheReadyToRetry;

	m_instanceIdLookupScratch.clear();
	m_tlasInstancesScratch.clear();
	if (!rebuildStaticCache)
	{
		PixScopedCpuEvent resolveStaticTlasEvent(L"DXR.ResolveStaticTLAS");
		for (const auto& cachedInstance : staticSceneData.tlasInstances)
		{
			auto*		   obj = scene->TryGetGameObject(GameObject::Handle::FromValue(cachedInstance.objectHandle));
			MeshComponent* meshComp = nullptr;
			if (nullptr != meshStorage)
			{
				meshComp = meshStorage->Get(HandleOf<MeshComponent>::FromValue(cachedInstance.meshComponentHandle));
			}
			auto* meshRes = meshComp ? meshComp->GetMeshResource() : nullptr;
			auto* blas = meshRes ? meshRes->GetBLAS() : nullptr;
			if (nullptr == obj || nullptr == meshComp || meshComp->GetMobility() != RenderMobility::Static ||
				meshComp->GetOwner().GetValue() != cachedInstance.objectHandle || nullptr == blas || !blas->IsBuilt() ||
				blas->GetGPUAddress() != cachedInstance.blasAddress)
			{
				rebuildStaticCache = true;
				m_tlasInstancesScratch.clear();
				break;
			}

			m_tlasInstancesScratch.push_back({obj, blas, cachedInstance.flags});
		}
	}

	if (rebuildStaticCache)
	{
		PixScopedCpuEvent staticEvent(L"DXR.RebuildStaticSceneCache");
		instanceData->syncBuffer.Clear();
		materialData->syncBuffer.Clear();
		materialData->terrainSurfaceSyncBuffer.Clear();
		geoTableData->syncBuffer.Clear();
		restirLightData->emissiveLightSync.Clear();
		restirLightData->emissiveLightCount = 0;
		restirLightData->emissiveLightWeightSum = 0.0f;
		materialData->materialToIndex.clear();

		bool allStaticResourcesReady = true;
		CollectMeshData(scene, m_tlasInstancesScratch, frameIndex, RenderMobility::Static, allStaticResourcesReady);

		auto instanceSpan = instanceData->syncBuffer.GetSpan();
		auto materialSpan = materialData->syncBuffer.GetSpan();
		auto terrainSurfaceSpan = materialData->terrainSurfaceSyncBuffer.GetSpan();
		auto geometrySpan = geoTableData->syncBuffer.GetSpan();
		auto emissiveLightSpan = restirLightData->emissiveLightSync.GetSpan();
		staticSceneData.instances.assign(instanceSpan.begin(), instanceSpan.end());
		staticSceneData.materials.assign(materialSpan.begin(), materialSpan.end());
		staticSceneData.terrainSurfaces.assign(terrainSurfaceSpan.begin(), terrainSurfaceSpan.end());
		staticSceneData.geometry.assign(geometrySpan.begin(), geometrySpan.end());
		staticSceneData.emissiveLights.assign(emissiveLightSpan.begin(), emissiveLightSpan.end());
		staticSceneData.emissiveLightWeightSum = restirLightData->emissiveLightWeightSum;
		staticSceneData.tlasInstances.clear();
		staticSceneData.tlasInstances.reserve(m_tlasInstancesScratch.size());
		for (const auto& instance : m_tlasInstancesScratch)
		{
			const auto objectHandle = instance.obj->GetHandle();
			const auto meshComponentHandle = instance.obj->GetComponentHandle<MeshComponent>();
			staticSceneData.tlasInstances.push_back(
				{objectHandle.GetValue(), meshComponentHandle.GetValue(), instance.blas->GetGPUAddress(), instance.flags
				}
			);
		}
		staticSceneData.instanceIdLookup = m_instanceIdLookupScratch;
		staticSceneData.materialToIndex = materialData->materialToIndex;
		m_staticSceneData.MarkDirty();

		staticSceneData.topologyHash = Utils::kFnv1a64OffsetBasis;
		staticSceneData.transformHash = Utils::kFnv1a64OffsetBasis;
		for (const auto& instance : m_tlasInstancesScratch)
		{
			HashTlasInstance(staticSceneData.topologyHash, staticSceneData.transformHash, instance);
		}
		const uint64_t staticSceneVersion = m_staticSceneData.Version();
		Utils::AppendFnv1a64(staticSceneData.topologyHash, &staticSceneVersion, sizeof(staticSceneVersion));
		staticSceneData.restirHistorySignatureSeed = BuildStaticRestirHistorySignature(
			std::span<const DxTLASInstance>{m_tlasInstancesScratch}, staticSceneVersion,
			std::span<const MaterialGPUData>{staticSceneData.materials},
			std::span<const TerrainSurfaceGPUData>{staticSceneData.terrainSurfaces},
			std::span<const GeoInfo>{staticSceneData.geometry},
			std::span<const RestirEmissiveLightData>{staticSceneData.emissiveLights}
		);

		staticSceneData.scene = scene;
		staticSceneData.meshRevision = meshRevision;
		staticSceneData.meshComponentCount = staticMeshComponentCount;
		staticSceneData.valid = true;
		staticSceneData.complete = allStaticResourcesReady && !pendingLoadsActive;
	}
	else
	{
		PixScopedCpuEvent restoreStaticCacheEvent(L"DXR.RestoreStaticSceneCache");
		{
			PixScopedCpuEvent instanceEvent(L"DXR.RestoreStaticSceneCache.InstanceTable");
			instanceData->syncBuffer.Assign(staticSceneData.instances);
		}
		{
			PixScopedCpuEvent materialEvent(L"DXR.RestoreStaticSceneCache.MaterialTable");
			materialData->syncBuffer.Assign(staticSceneData.materials);
		}
		{
			PixScopedCpuEvent terrainEvent(L"DXR.RestoreStaticSceneCache.TerrainSurfaceTable");
			materialData->terrainSurfaceSyncBuffer.Assign(staticSceneData.terrainSurfaces);
		}
		{
			PixScopedCpuEvent geometryEvent(L"DXR.RestoreStaticSceneCache.GeometryTable");
			geoTableData->syncBuffer.Assign(staticSceneData.geometry);
		}
		{
			PixScopedCpuEvent emissiveEvent(L"DXR.RestoreStaticSceneCache.EmissiveLightTable");
			restirLightData->emissiveLightSync.Assign(staticSceneData.emissiveLights);
			restirLightData->emissiveLightCount = static_cast<uint32_t>(staticSceneData.emissiveLights.size());
			restirLightData->emissiveLightWeightSum = staticSceneData.emissiveLightWeightSum;
		}
		{
			PixScopedCpuEvent lookupEvent(L"DXR.RestoreStaticSceneCache.LookupTables");
			materialData->materialToIndex = staticSceneData.materialToIndex;
			m_instanceIdLookupScratch = staticSceneData.instanceIdLookup;
		}
	}
	staticSceneData.pendingLoadsActive = pendingLoadsActive;

	{
		PixScopedCpuEvent movableEvent(L"DXR.CollectMovableMeshData");
		bool			  allMovableResourcesReady = true;
		CollectMeshData(scene, m_tlasInstancesScratch, frameIndex, RenderMobility::Movable, allMovableResourcesReady);
	}
	{
		PixScopedCpuEvent skinnedEvent(L"DXR.CollectSkinnedMeshData");
		CollectSkinnedMeshData(
			scene, cmdList4.Get(), m_tlasInstancesScratch, frameIndex, hasAnimatedInstances, animatedBlasCount
		);
	}
	m_lastAnimatedBlasCount = animatedBlasCount;

	instanceData->tlasDescriptorIndex = 0;
	instanceData->tlasAddress = 0;
	if (m_tlasInstancesScratch.empty())
	{
		if (nullptr != tlas)
		{
			tlas->Invalidate();
		}
		tlasFrame.lastInstanceCount = 0;
		tlasFrame.lastTopologyHash = 0;
		tlasFrame.lastTransformHash = 0;
		return;
	}

	if (nullptr != tlas)
	{
		uint64_t topologyHash = staticSceneData.topologyHash;
		uint64_t transformHash = staticSceneData.transformHash;
		for (size_t i = staticSceneData.tlasInstances.size(); i < m_tlasInstancesScratch.size(); ++i)
		{
			HashTlasInstance(topologyHash, transformHash, m_tlasInstancesScratch[i]);
		}

		const uint32_t staticInstanceCount = static_cast<uint32_t>(staticSceneData.tlasInstances.size());
		const uint32_t currentInstanceCount = static_cast<uint32_t>(m_tlasInstancesScratch.size());
		const bool	   topologyChanged = !tlas->IsBuilt() || tlas->GetInstanceCount() != currentInstanceCount ||
									 tlasFrame.lastInstanceCount != currentInstanceCount ||
									 tlasFrame.lastTopologyHash != topologyHash;
		const bool transformsChanged = tlasFrame.lastTransformHash != transformHash;

		if (topologyChanged)
		{
			PixScopedCpuEvent buildCpuEvent(L"DXR.BuildTLAS.CPU");
			DxScopedGpuEvent  buildGpuEvent(context, L"DXR.BuildTLAS.GPU");
			tlas->Build(
				m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), m_tlasInstancesScratch, staticInstanceCount
			);
		}
		else if (transformsChanged || hasAnimatedInstances)
		{
			PixScopedCpuEvent refitCpuEvent(L"DXR.RefitTLAS.CPU");
			DxScopedGpuEvent  refitGpuEvent(context, L"DXR.RefitTLAS.GPU");
			tlas->Refit(
				m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), m_tlasInstancesScratch, staticInstanceCount
			);
		}

		if (!tlas->IsBuilt())
		{
			tlasFrame.lastInstanceCount = 0;
			tlasFrame.lastTopologyHash = 0;
			tlasFrame.lastTransformHash = 0;
			return;
		}

		tlasFrame.lastInstanceCount = currentInstanceCount;
		tlasFrame.lastTopologyHash = topologyHash;
		tlasFrame.lastTransformHash = transformHash;
		instanceData->tlasDescriptorIndex = tlas->GetSRVIndex();
		instanceData->tlasAddress = tlas->GetGPUAddress();
	}
}

void RaytracingPreparePass::CollectMeshData(
	Scene*						 scene,
	std::vector<DxTLASInstance>& tlasInstances,
	uint32_t					 frameIndex,
	RenderMobility				 mobility,
	bool&						 allResourcesReady
)
{
	PixScopedCpuEvent event(
		mobility == RenderMobility::Static ? L"DXR.CollectStaticMeshes" : L"DXR.CollectMovableMeshes"
	);

	auto* meshStorage = scene->GetStorage<MeshComponent>();
	if (nullptr == meshStorage)
	{
		return;
	}

	auto* geoTableData = &m_geoTableData[frameIndex];
	auto* instanceData = &m_instanceData[frameIndex];

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (meshComp.GetMobility() != mobility)
		{
			continue;
		}

		if (false == meshComp.IsValid())
		{
			allResourcesReady = false;
			continue;
		}

		auto* meshRes = meshComp.GetMeshResource();
		if (nullptr == meshRes || !meshRes->IsReady() || nullptr == meshRes->GetVertexBuffer() ||
			nullptr == meshRes->GetIndexBuffer())
		{
			allResourcesReady = false;
			continue;
		}

		if (0 == meshRes->GetVertexBuffer()->GetGPUAddress() || 0 == meshRes->GetIndexBuffer()->GetGPUAddress())
		{
			allResourcesReady = false;
			continue;
		}

		auto* blas = meshRes->GetBLAS();
		if (nullptr == blas || false == blas->IsBuilt() || 0 == blas->GetGPUAddress())
		{
			allResourcesReady = false;
			continue;
		}

		const uint32_t ownerId = meshComp.GetOwner().id;
		const uint32_t mappedInstanceIndex = static_cast<uint32_t>(instanceData->syncBuffer.Size());
		uint32_t	   geoBaseIdx = static_cast<uint32_t>(geoTableData->syncBuffer.Size());
		bool		   disableTriangleCull = false;
		const auto&	   subMeshes = meshRes->GetSubMeshes();
		for (uint32_t subMeshIndex = 0; subMeshIndex < static_cast<uint32_t>(subMeshes.size()); ++subMeshIndex)
		{
			const auto& subMesh = subMeshes[subMeshIndex];
			auto*		matRes = meshComp.GetMaterial(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}
			disableTriangleCull |= (0 != (matRes->GetMaterialFlags() & MATERIAL_FLAG_DOUBLE_SIDED));


			const auto&	   meshGuid = meshRes->GetGuid();
			const bool	   isRuntimeMesh = meshGuid.IsNull();
			const uint32_t stableGeometryId =
				isRuntimeMesh ? MakeStableRuntimeId(kStableStaticRuntimeGeometryIdSalt, ownerId, subMeshIndex)
							  : MakeStableAssetId(kStableStaticAssetGeometryIdSalt, meshGuid, subMeshIndex);

			const uint32_t matIdx = RegisterMaterial(matRes, ownerId, subMesh.materialSlot, frameIndex);
			RegisterGeometry(subMesh, matRes, matIdx, stableGeometryId, mappedInstanceIndex, subMeshIndex, frameIndex);
		}

		InstanceData inst = MakeInstanceData(
			meshComp.GetGameObject(), meshRes->GetVertexBuffer()->GetSRVIndex(),
			meshRes->GetIndexBuffer()->GetSRVIndex(), geoBaseIdx
		);
		if (RenderMobility::Static == mobility)
		{
			InitializeStaticInstanceMotion(inst);
		}
		else
		{
			ApplyInstanceMotionHistory(inst, false, inst.vertexBufferIdx);
		}

		instanceData->syncBuffer.Register(inst);
		RegisterInstanceIdLookup(ownerId, mappedInstanceIndex);
		tlasInstances.push_back(
			{meshComp.GetGameObject(), blas,
			 disableTriangleCull ? D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
								 : D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		);
	}
}

uint32_t RaytracingPreparePass::RegisterTerrainSurface(MaterialRenderData* materialData, MaterialResource* material)
	const
{
	if (nullptr == materialData || nullptr == material)
	{
		return ~0u;
	}

	const auto			  terrainSize = material->GetTerrainSize();
	const auto*			  terrainLayerTileST = material->GetTerrainLayerTileST();
	TerrainSurfaceGPUData surface = {};
	surface.splatTextureIdx = GetReadyTextureIndex(material, "SPL0");
	surface.layerAlbedoTextureIdx0 = GetReadyTextureIndex(material, "LA0A");
	surface.layerAlbedoTextureIdx1 = GetReadyTextureIndex(material, "LA1A");
	surface.layerAlbedoTextureIdx2 = GetReadyTextureIndex(material, "LA2A");
	surface.layerAlbedoTextureIdx3 = GetReadyTextureIndex(material, "LA3A");
	surface.layerNormalTextureIdx0 = GetReadyTextureIndex(material, "LA0N");
	surface.layerNormalTextureIdx1 = GetReadyTextureIndex(material, "LA1N");
	surface.layerNormalTextureIdx2 = GetReadyTextureIndex(material, "LA2N");
	surface.layerNormalTextureIdx3 = GetReadyTextureIndex(material, "LA3N");
	surface.layerOrmTextureIdx0 = GetReadyTextureIndex(material, "LA0O");
	surface.layerOrmTextureIdx1 = GetReadyTextureIndex(material, "LA1O");
	surface.layerOrmTextureIdx2 = GetReadyTextureIndex(material, "LA2O");
	surface.layerOrmTextureIdx3 = GetReadyTextureIndex(material, "LA3O");
	surface.layerCount = material->GetTerrainLayerCount();
	surface.terrainSize = {terrainSize.x, terrainSize.y, 0.0f, 0.0f};

	const auto* terrainLayerMetallicRoughness = material->GetTerrainLayerMetallicRoughness();
	for (uint32_t i = 0; i < 4; ++i)
	{
		surface.layerTileST[i] = terrainLayerTileST[i];
		surface.layerMetallicRoughness[i] = {
			terrainLayerMetallicRoughness[i].x, terrainLayerMetallicRoughness[i].y, 0.0f, 0.0f
		};
	}

	uint32_t surfaceIdx = static_cast<uint32_t>(materialData->terrainSurfaceSyncBuffer.Size());
	materialData->terrainSurfaceSyncBuffer.Register(surface);
	return surfaceIdx;
}

void RaytracingPreparePass::CollectSkinnedMeshData(
	Scene*						 scene,
	ID3D12GraphicsCommandList4*	 cmdList,
	std::vector<DxTLASInstance>& tlasInstances,
	uint32_t					 frameIndex,
	bool&						 hasAnimatedInstances,
	uint32_t&					 animatedBlasCount
)
{
	PixScopedCpuEvent event(L"DXR.CollectSkinnedMeshes");

	auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
	if (nullptr == skinnedMeshStorage)
	{
		return;
	}

	auto* geoTableData = &m_geoTableData[frameIndex];
	auto* instanceData = &m_instanceData[frameIndex];

	for (const auto& skinnedMeshComp : skinnedMeshStorage->GetList())
	{
		if (false == skinnedMeshComp.IsValid())
		{
			continue;
		}

		auto* meshRes = skinnedMeshComp.GetSkinnedMeshResource();
		auto* skinnedVB = skinnedMeshComp.GetSkinnedVertexBuffer(frameIndex);
		if (nullptr == meshRes || !meshRes->IsReady() || nullptr == meshRes->GetIndexBuffer() || nullptr == skinnedVB)
		{
			continue;
		}

		if (0 == skinnedVB->GetGPUAddress() || 0 == meshRes->GetIndexBuffer()->GetGPUAddress())
		{
			continue;
		}

		auto*	   blas = skinnedMeshComp.GetBLAS(frameIndex);
		const bool newBlas = nullptr == blas;
		if (newBlas)
		{
			const_cast<SkinnedMeshComponent&>(skinnedMeshComp).SetBLAS(frameIndex, std::make_unique<DxBLAS>());
			blas = skinnedMeshComp.GetBLAS(frameIndex);
		}
		if (!blas->IsBuilt())
		{
			PixScopedCommandListEvent blasEvent(
				cmdList, newBlas ? L"DXR.BuildAnimatedBLAS" : L"DXR.RebuildAnimatedBLAS"
			);
			blas->Build(
				m_device5.Get(), cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(),
				sizeof(EvAsset::Vertex), meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetIndexCount(),
				meshRes->GetSubMeshes(), true, meshRes->GetName() + "_AnimatedBLAS_" + std::to_string(frameIndex)
			);
		}
		else
		{
			PixScopedCommandListEvent blasEvent(cmdList, L"DXR.RefitAnimatedBLAS");
			blas->Refit(
				cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(), sizeof(EvAsset::Vertex),
				meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetIndexCount(), meshRes->GetSubMeshes()
			);
		}

		if (nullptr == blas || false == blas->IsBuilt() || 0 == blas->GetGPUAddress())
		{
			continue;
		}

		hasAnimatedInstances = true;
		++animatedBlasCount;

		const uint32_t ownerId = skinnedMeshComp.GetOwner().id;
		const uint32_t mappedInstanceIndex = static_cast<uint32_t>(instanceData->syncBuffer.Size());
		uint32_t	   geoBaseIdx = static_cast<uint32_t>(geoTableData->syncBuffer.Size());
		bool		   disableTriangleCull = false;
		const auto&	   subMeshes = meshRes->GetSubMeshes();
		for (uint32_t subMeshIndex = 0; subMeshIndex < static_cast<uint32_t>(subMeshes.size()); ++subMeshIndex)
		{
			const auto& subMesh = subMeshes[subMeshIndex];
			auto*		matRes = skinnedMeshComp.GetMaterialResource(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}
			disableTriangleCull |= (0 != (matRes->GetMaterialFlags() & MATERIAL_FLAG_DOUBLE_SIDED));


			const auto&	   meshGuid = meshRes->GetGuid();
			const bool	   isRuntimeMesh = meshGuid.IsNull();
			const uint32_t stableGeometryId =
				isRuntimeMesh ? MakeStableRuntimeId(kStableSkinnedRuntimeGeometryIdSalt, ownerId, subMeshIndex)
							  : MakeStableAssetId(kStableSkinnedAssetGeometryIdSalt, meshGuid, subMeshIndex);

			const uint32_t matIdx = RegisterMaterial(matRes, ownerId, subMesh.materialSlot, frameIndex);
			RegisterGeometry(subMesh, matRes, matIdx, stableGeometryId, mappedInstanceIndex, subMeshIndex, frameIndex);
		}

		InstanceData inst = MakeInstanceData(
			skinnedMeshComp.GetGameObject(), skinnedVB->GetSRVIndex(), meshRes->GetIndexBuffer()->GetSRVIndex(),
			geoBaseIdx
		);

		uint32_t previousVertexBufferIndex = 0xffffffffu;
		if (m_hasPreviousInstanceFrame && m_previousInstanceFrameIndex != frameIndex)
		{
			auto* previousSkinnedVB = skinnedMeshComp.GetSkinnedVertexBuffer(m_previousInstanceFrameIndex);
			if (nullptr != previousSkinnedVB && previousSkinnedVB->HasSRV())
			{
				previousVertexBufferIndex = previousSkinnedVB->GetSRVIndex();
			}
		}
		ApplyInstanceMotionHistory(inst, true, previousVertexBufferIndex);

		instanceData->syncBuffer.Register(inst);
		RegisterInstanceIdLookup(ownerId, mappedInstanceIndex);
		tlasInstances.push_back(
			{skinnedMeshComp.GetGameObject(), blas,
			 disableTriangleCull ? D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
								 : D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		);
	}
}


void RaytracingPreparePass::Initialize()
{
	ThrowIfFailed(GLOBAL(DxDeviceGlobal).GetDevice()->QueryInterface(IID_PPV_ARGS(&m_device5)));
	for (uint32_t i = 0; i < 3; ++i)
	{
		m_tlas[i].Initialize(m_device5.Get());
	}

	for (uint32_t i = 0; i < 3; ++i)
	{
		m_outputData[i].outputTexture = std::make_shared<DxTexture>();
	}
	CreateOutputResources(m_width, m_height);
	m_initialized = true;
}

void RaytracingPreparePass::Release()
{
	m_tlas.Release();
	m_instanceData.Release();
	m_materialData.Release();
	m_geoTableData.Release();
	m_restirLightData.Release();
	m_staticSceneData.Release();
	m_tlasInstancesScratch.clear();
	m_instanceIdLookupScratch.clear();
	m_previousInstanceWorldMatrices.clear();
	m_currentInstanceWorldMatrices.clear();
	m_instanceMotionScene = nullptr;
	m_hasPreviousInstanceFrame = false;
	m_hasPreviousViewProj = false;
	m_device5.Reset();

	m_outputData.Release();
	m_frameData.Release();
	m_initialized = false;
}

void RaytracingPreparePass::BeginFrame(uint32_t frameIndex, Scene* scene)
{
	BeginInstanceMotionFrame(scene);
	m_instanceData.BeginFrame(frameIndex);
	m_materialData.BeginFrame(frameIndex);
	m_geoTableData.BeginFrame(frameIndex);
	m_restirLightData.BeginFrame(frameIndex);
	m_tlas.BeginFrame(frameIndex);
	m_staticSceneData.BeginFrame(frameIndex);
}

void RaytracingPreparePass::Prepare(
	DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition, bool buildInstanceLookup
)
{
	PrepareRenderData(frame, scene, cameraPosition);
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& context = *frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();
	auto* instanceData = &m_instanceData.GetCurrent();
	auto* materialData = &m_materialData.GetCurrent();
	auto* geoTableData = &m_geoTableData.GetCurrent();
	auto* restirLightData = &m_restirLightData.GetCurrent();
	{
		PixScopedCpuEvent fallbackEvent(L"DXR.EnsureFallbackRenderData");
		restirLightData->emissiveLightCount = static_cast<uint32_t>(restirLightData->emissiveLightSync.Size());
		if (0 == materialData->terrainSurfaceSyncBuffer.Size())
		{
			materialData->terrainSurfaceSyncBuffer.Register(TerrainSurfaceGPUData{});
		}
		if (0 == restirLightData->emissiveLightCount)
		{
			restirLightData->emissiveLightSync.Register(RestirEmissiveLightData{});
		}
	}
	{
		PixScopedCpuEvent syncCpuEvent(L"DXR.SyncRenderData");
		DxScopedGpuEvent  syncEvent(context, L"DXR.SyncRenderData");
		instanceData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		materialData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		materialData->terrainSurfaceSyncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		geoTableData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		restirLightData->emissiveLightSync.SyncToGPU(device.GetDevice(), context, *uploadHeap);

		if (buildInstanceLookup && !m_instanceIdLookupScratch.empty())
		{
			auto& lookupSync = instanceData->idToInstanceIndexSync;
			lookupSync.Assign(m_instanceIdLookupScratch);
			lookupSync.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		}
		m_staticSceneData.ClearDirty();
	}
}

void RaytracingPreparePass::Publish(RenderContext* renderContext)
{
	renderContext->Set(m_instanceData);
	renderContext->Set(m_materialData);
	renderContext->Set(m_geoTableData);
	renderContext->Set(m_restirLightData);
	renderContext->Set(m_tlas);
}

void RaytracingPreparePass::CommitRenderedFrame()
{
	auto& previous = m_frameData.Get();
	if (!previous.dispatched)
	{
		return;
	}
	if (previous.hasCamera)
	{
		m_previousViewProj = previous.viewProjection;
		m_hasPreviousViewProj = true;
	}
	CommitInstanceMotionFrame(previous.frameIndex);
	previous.dispatched = false;
	++m_frameSeed;
}

void RaytracingPreparePass::OnEndFrame(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!frame || !renderContext || renderContext->Get<RaytracingFrameRenderData>() != &m_frameData.Get())
	{
		return;
	}
	CommitRenderedFrame();
}

uint64_t RaytracingPreparePass::BuildHistorySignature(uint64_t generation) const
{
	return BuildRestirHistorySignature(
		std::span<const DxTLASInstance>{m_tlasInstancesScratch}, m_staticSceneData.Get(), m_materialData.GetCurrent(),
		m_geoTableData.GetCurrent(), m_restirLightData.GetCurrent(), generation
	);
}

uint32_t RaytracingPreparePass::RegisterMaterial(
	MaterialResource* material, uint32_t ownerId, uint32_t materialSlot, uint32_t frameIndex
)
{
	auto*		   materialData = &m_materialData[frameIndex];
	uint32_t	   matIdx = 0;
	const auto&	   matGuid = material->GetGuid();
	const bool	   isRuntimeMaterial = matGuid.IsNull();
	const uint32_t stableMaterialId = isRuntimeMaterial
										  ? MakeStableRuntimeId(kStableRuntimeMaterialIdSalt, ownerId, materialSlot)
										  : MakeStableAssetId(kStableAssetMaterialIdSalt, matGuid);
	if (!isRuntimeMaterial && materialData->materialToIndex.contains(matGuid))
	{
		matIdx = materialData->materialToIndex[matGuid];
	}
	else
	{
		matIdx = static_cast<uint32_t>(materialData->syncBuffer.Size());
		MaterialGPUData gpuMat = {};
		gpuMat.albedo = material->GetAlbedo();
		gpuMat.roughness = material->GetRoughness();
		gpuMat.metallic = material->GetMetallic();
		gpuMat.emissive = material->GetEmissive();
		gpuMat.visibleEmissive = material->GetVisibleEmissive();
		gpuMat.shadingModel = static_cast<uint32_t>(material->GetShadingModelId());
		gpuMat.materialFlags = material->GetMaterialFlags();
		gpuMat.albedoTextureIdx = GetReadyTextureIndex(material, "ALBD");
		gpuMat.normalTextureIdx = GetReadyTextureIndex(material, "NRML");
		gpuMat.ormTextureIdx = GetReadyTextureIndex(material, "ORMS");
		gpuMat.emissiveTextureIdx = GetReadyTextureIndex(material, "EMSV");
		gpuMat.terrainSurfaceIdx = (0 != (material->GetMaterialFlags() & MATERIAL_FLAG_TERRAIN_SPLAT))
									   ? RegisterTerrainSurface(materialData, material)
									   : ~0u;
		gpuMat.stableMaterialId = stableMaterialId;

		materialData->syncBuffer.Register(gpuMat);
		if (!isRuntimeMaterial)
		{
			materialData->materialToIndex[matGuid] = matIdx;
		}
	}

	return matIdx;
}

void RaytracingPreparePass::RegisterGeometry(
	const EvAsset::SubMesh& subMesh,
	MaterialResource*		material,
	uint32_t				materialIndex,
	uint32_t				stableGeometryId,
	uint32_t				instanceIndex,
	uint32_t				subMeshIndex,
	uint32_t				frameIndex
)
{
	auto*		   geoTableData = &m_geoTableData[frameIndex];
	auto*		   restirLightData = &m_restirLightData[frameIndex];
	const bool	   isEmissiveLight = IsRestirEmissiveLightMaterial(material) && subMesh.indexCount >= 3;
	const uint32_t triangleCount = subMesh.indexCount / 3u;
	const uint32_t emissiveEntryIdx =
		isEmissiveLight ? static_cast<uint32_t>(restirLightData->emissiveLightSync.Size()) : ~0u;

	geoTableData->syncBuffer.Register(
		{.vertexBase = 0,
		 .indexBase = subMesh.indexOffset,
		 .materialIdx = materialIndex,
		 .stableGeometryId = stableGeometryId,
		 .emissiveEntryIdx = emissiveEntryIdx}
	);
	if (isEmissiveLight)
	{
		const float selectionWeight = EstimateRestirEmissiveLightSelectionWeight(material, triangleCount);
		const float cumulativeWeight =
			AppendRestirEmissiveLightCdf(restirLightData->emissiveLightWeightSum, selectionWeight);
		restirLightData->emissiveLightSync.Register(
			{.instanceIndex = instanceIndex,
			 .geometryIndex = subMeshIndex,
			 .triangleCount = triangleCount,
			 .cumulativeWeight = cumulativeWeight}
		);
	}
}

void RaytracingPreparePass::DeclareRenderData(RenderContext* renderContext)
{
	if (nullptr == renderContext)
	{
		return;
	}

	renderContext->DeclareAccess<FrameRenderData>(GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read);
	renderContext->DeclareAccess<CameraRenderData>(GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read);
	renderContext->DeclareAccess<InstanceRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<MaterialRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<GeoTableRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<RestirLightRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<TlasRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<RaytracingOutputRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);

	renderContext->DeclareAccess<RaytracingFrameRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Write
	);
}

void RaytracingPreparePass::CreateOutputResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	for (int i = 0; i < 3; ++i)
	{
		auto& outputTex = m_outputData[i].outputTexture;

		if (outputTex->HasUAV(0))
		{
			outputTex->ReleaseAllViews(descHeap);
		}

		outputTex->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			"DxrRenderPass_Output_" + std::to_string(i)
		);
		outputTex->CreateSRV(device.GetDevice(), descHeap);
		outputTex->CreateUAV(device.GetDevice(), descHeap, 0);

		GRAPHICS_LOG_FMT(
			"[DxrRenderPass] Raytracing output {} created: {}x{}, UAV Index={}\n", i, width, height,
			outputTex->GetUAVIndex(0)
		);
	}
}
RaytracingPreparePass::RaytracingPreparePass(uint32_t width, uint32_t height) : m_width(width), m_height(height) {}

void RaytracingPreparePass::OnResize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	m_hasPreviousViewProj = false;
	m_frameData.Get().ready = false;
	CreateOutputResources(width, height);
}

void RaytracingPreparePass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!m_initialized || !frame || !scene || !renderContext)
	{
		return;
	}
	const uint32_t frameIndex = frame->GetFrameIndex();
	BeginFrame(frameIndex, scene);
	m_outputData.BeginFrame(frameIndex);
	m_frameData.BeginFrame(frameIndex);
	auto& frameData = m_frameData.Get();
	frameData.ready = false;
	frameData.dispatched = false;
	frameData.frameIndex = frameIndex;
	frameData.frameSeed = m_frameSeed;
	frameData.historySignature = 0;
	m_controls.Update();
	frameData.path = m_controls.GetPath();
	frameData.settings = m_controls.GetSettings();
	auto& output = m_outputData.GetCurrent();
	m_controls.ApplyOutputSettings(output);
	Publish(renderContext);
	renderContext->Set(m_outputData);
	renderContext->Set(m_frameData);
	if (!output.outputTexture || !output.outputTexture->HasUAV(0))
	{
		return;
	}
	auto* cameraData = renderContext->Get<CameraRenderData>();
	Prepare(
		frame, scene, cameraData ? &cameraData->cameraPosition : nullptr,
		frameData.path == RaytracingPath::RestirCandidate
	);
	auto* tlas = m_tlas.GetCurrent().Get();
	if (!tlas || !tlas->IsBuilt())
	{
		return;
	}
	const auto viewProjection = cameraData
									? DirectX::XMMatrixMultiply(cameraData->viewMatrix, cameraData->projectionMatrix)
									: DirectX::XMMatrixIdentity();
	DirectX::XMStoreFloat4x4(&frameData.viewProjection, viewProjection);
	frameData.previousViewProjection = m_hasPreviousViewProj ? m_previousViewProj : frameData.viewProjection;
	frameData.hasCamera = cameraData != nullptr;
	frameData.hasPreviousFrame = m_hasPreviousViewProj && m_hasPreviousInstanceFrame;
	if (frameData.path == RaytracingPath::RestirCandidate)
	{
		frameData.historySignature = BuildHistorySignature(frameData.settings.restirHistoryGeneration);
	}
	frameData.animatedBlasCount = m_lastAnimatedBlasCount;
	frameData.instanceCount = static_cast<uint32_t>(m_tlasInstancesScratch.size());
	frameData.staticInstanceCount = static_cast<uint32_t>(m_staticSceneData.Get().tlasInstances.size());
	frameData.ready = true;
}
