#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include "Scene.h"
#include "DxFrameResource.h"
#include "DxCommandContext.h"
#include "PixProfiler.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxSamplerHeapGlobal.h"
#include "DxRendererGlobal.h"
#include "DxDeviceGlobal.h"
#include "ResourceGlobal.h"
#include "InputGlobal.h"
#include "MeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "DxBLAS.h"
#include "DxBuffer.h"
#include "DxUtils.h"
#include "MeshResource.h"
#include "MaterialResource.h"
#include "TextureResource.h"
#include "CameraRenderData.h"
#include "FrameRenderData.h"
#include "RaytracingCommon.h"

#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace
{
constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

void HashBytes(uint64_t& hash, const void* data, size_t size)
{
	const auto* bytes = static_cast<const uint8_t*>(data);
	for (size_t i = 0; i < size; ++i)
	{
		hash ^= bytes[i];
		hash *= kFnvPrime;
	}
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

uint32_t RegisterTerrainSurface(MaterialRenderData* materialData, MaterialResource* material)
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

bool IsNullGuid(const EvAsset::Guid& guid)
{
	for (uint8_t byte : guid.data)
	{
		if (byte != 0)
		{
			return false;
		}
	}
	return true;
}

void TransitionResourceIfNeeded(
	ID3D12GraphicsCommandList* cmdList, DxResource* resource, D3D12_RESOURCE_STATES targetState
)
{
	if (nullptr == cmdList || nullptr == resource || nullptr == resource->GetResource())
	{
		return;
	}

	const auto currentState = resource->GetCurrentState();
	if (currentState == targetState)
	{
		return;
	}

	auto barrier = DxUtils::CreateTransitionBarrier(resource->GetResource(), currentState, targetState);
	cmdList->ResourceBarrier(1, &barrier);
	resource->SetState(targetState);
}
} // namespace

DxrRenderPass::DxrRenderPass(uint32_t width, uint32_t height) : m_width(width), m_height(height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Constructor: {}x{}\n", width, height);
}

void DxrRenderPass::Initialize()
{
	DEBUG_LOG_FMT("[DxrRenderPass] Initializing with resolution: {}x{}\n", m_width, m_height);

	auto& deviceG = GLOBAL(DxDeviceGlobal);
	auto* device = deviceG.GetDevice();
	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_device5)));

	for (int i = 0; i < 3; ++i)
	{
		m_tlas[i] = std::make_unique<DxTLAS>();
		m_tlas[i]->Initialize(m_device5.Get());

		m_instanceData[i] = std::make_shared<InstanceRenderData>();
		m_materialData[i] = std::make_shared<MaterialRenderData>();
		m_geoTableData[i] = std::make_shared<GeoTableRenderData>();

		m_outputData[i] = std::make_shared<RaytracingOutputRenderData>();
		m_outputData[i]->outputTexture = std::make_shared<DxTexture>();
	}
	for (auto& history : m_ptAccumHistory)
	{
		history = std::make_shared<DxTexture>();
	}

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	DEBUG_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	for (int i = 0; i < 3; ++i)
	{
		m_tlas[i].reset();
		m_instanceData[i].reset();
		m_materialData[i].reset();
		m_geoTableData[i].reset();
		m_outputData[i].reset();
	}
	for (auto& history : m_ptAccumHistory)
	{
		history.reset();
	}
	m_device5.Reset();

	m_initialized = false;
	DEBUG_LOG_FMT("[DxrRenderPass] Released\n");
}

void DxrRenderPass::OnResize(uint32_t width, uint32_t height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Resizing: {}x{} -> {}x{}\n", m_width, m_height, width, height);

	m_width = width;
	m_height = height;

	CreateRaytracingResources(m_width, m_height);
	ResetTemporalAccumulation();
}

void DxrRenderPass::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	for (int i = 0; i < 3; ++i)
	{
		auto& outputTex = m_outputData[i]->outputTexture;

		if (outputTex->HasUAV(0))
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			outputTex->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
		}

		outputTex->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "DxrRenderPass_Output_" + std::to_string(i)
		);
		outputTex->CreateUAV(device.GetDevice(), descHeap, 0);

		DEBUG_LOG_FMT(
			"[DxrRenderPass] Raytracing output {} created: {}x{}, UAV Index={}\n", i, width, height,
			outputTex->GetUAVIndex(0)
		);
	}

	for (int i = 0; i < 2; ++i)
	{
		auto& historyTex = m_ptAccumHistory[i];
		if (!historyTex)
		{
			historyTex = std::make_shared<DxTexture>();
		}

		if (historyTex->HasSRV() || historyTex->HasUAV(0))
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			historyTex->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
		}

		historyTex->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			"DxrRenderPass_PTAccumHistory_" + std::to_string(i)
		);
		historyTex->CreateSRV(device.GetDevice(), descHeap);
		historyTex->CreateUAV(device.GetDevice(), descHeap, 0);
	}

	ResetTemporalAccumulation();
}

void DxrRenderPass::ResetTemporalAccumulation()
{
	m_temporalAccumulationResetPending = true;
	m_temporalAccumulationFrameCount = 0;
	m_temporalAccumulationReadIndex = 0;
	m_temporalAccumulationWriteIndex = 1;
	m_hasTemporalCamera = false;
}

bool DxrRenderPass::ShouldResetTemporalAccumulation(const CameraRenderData* cameraData) const
{
	if (m_temporalAccumulationResetPending || nullptr == cameraData || !m_hasTemporalCamera)
	{
		return true;
	}

	const float dx = cameraData->cameraPosition.x - m_lastTemporalCameraPosition.x;
	const float dy = cameraData->cameraPosition.y - m_lastTemporalCameraPosition.y;
	const float dz = cameraData->cameraPosition.z - m_lastTemporalCameraPosition.z;
	const float directionDx = cameraData->cameraDirection.x - m_lastTemporalCameraDirection.x;
	const float directionDy = cameraData->cameraDirection.y - m_lastTemporalCameraDirection.y;
	const float directionDz = cameraData->cameraDirection.z - m_lastTemporalCameraDirection.z;
	const float positionDeltaSq = dx * dx + dy * dy + dz * dz;
	const float directionDeltaSq = directionDx * directionDx + directionDy * directionDy + directionDz * directionDz;
	if (positionDeltaSq > 0.0001f || directionDeltaSq > 0.000001f)
	{
		return true;
	}

	if (std::abs(cameraData->fov - m_lastTemporalFov) > 0.0001f ||
		std::abs(cameraData->aspectRatio - m_lastTemporalAspectRatio) > 0.0001f)
	{
		return true;
	}

	return false;
}

void DxrRenderPass::PrepareRenderData(DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition)
{
	PixScopedCpuEvent cpuEvent(L"DXR.PrepareRenderData");
	auto&			 context = *frame->GetMainContext();

	const uint32_t frameIndex = frame->GetFrameIndex();
	auto&		   instanceData = m_instanceData[frameIndex];
	auto&		   materialData = m_materialData[frameIndex];
	auto&		   geoTableData = m_geoTableData[frameIndex];
	auto&		   tlas = m_tlas[frameIndex];
	bool		   hasAnimatedInstances = false;

	instanceData->syncBuffer.Clear();
	materialData->syncBuffer.Clear();
	materialData->terrainSurfaceSyncBuffer.Clear();
	geoTableData->syncBuffer.Clear();
	materialData->materialToIndex.clear();

	std::vector<DxTLASInstance> tlasInstances;
	tlasInstances.reserve(1'000);

	auto*							   cmdList = context.CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	{
		PixScopedCpuEvent staticEvent(L"DXR.CollectStaticMeshData");
		CollectStaticMeshData(scene, cmdList4.Get(), tlasInstances, frameIndex);
	}
	{
		PixScopedCpuEvent skinnedEvent(L"DXR.CollectSkinnedMeshData");
		CollectSkinnedMeshData(scene, cmdList4.Get(), tlasInstances, frameIndex, hasAnimatedInstances);
	}

	if (nullptr != tlas && false == tlasInstances.empty())
	{
		struct TlasTopologyKey
		{
			uint32_t objectId;
			uint64_t blasAddr;
			uint32_t flags;
		};
		std::vector<TlasTopologyKey> topologyKeys;
		topologyKeys.reserve(tlasInstances.size());
		for (const auto& instance : tlasInstances)
		{
			const auto*	   obj = instance.obj;
			const auto*	   blas = instance.blas;
			const uint32_t objectId = obj ? obj->GetHandle().id : 0u;
			const uint64_t blasAddr = blas ? blas->GetGPUAddress() : 0ull;
			const uint32_t instanceFlags = static_cast<uint32_t>(instance.flags);
			topologyKeys.push_back({objectId, blasAddr, instanceFlags});
		}

		std::sort(
			topologyKeys.begin(), topologyKeys.end(),
			[](const auto& lhs, const auto& rhs)
			{
				if (lhs.objectId != rhs.objectId)
				{
					return lhs.objectId < rhs.objectId;
				}
				if (lhs.blasAddr != rhs.blasAddr)
				{
					return lhs.blasAddr < rhs.blasAddr;
				}
				return lhs.flags < rhs.flags;
			}
		);

		uint64_t topologyHash = kFnvOffsetBasis;
		for (const auto& key : topologyKeys)
		{
			HashBytes(topologyHash, &key.objectId, sizeof(key.objectId));
			HashBytes(topologyHash, &key.blasAddr, sizeof(key.blasAddr));
			HashBytes(topologyHash, &key.flags, sizeof(key.flags));
		}

		const uint32_t currentInstanceCount = static_cast<uint32_t>(tlasInstances.size());
		if (m_lastTlasInstanceCount[frameIndex] == currentInstanceCount)
		{
			++m_tlasStableFrameCount[frameIndex];
		}
		else
		{
			m_lastTlasInstanceCount[frameIndex] = currentInstanceCount;
			m_tlasStableFrameCount[frameIndex] = 0;
		}

		const bool pendingLoadsActive = GLOBAL(ResourceGlobal).HasPendingLoads();
		const bool canRefit = tlas->IsBuilt() && (tlas->GetInstanceCount() == currentInstanceCount) &&
							  (m_lastTlasTopologyHash[frameIndex] == topologyHash) && !pendingLoadsActive &&
							  (m_tlasStableFrameCount[frameIndex] >= 2);

		if (!canRefit)
		{
			DxScopedGpuEvent buildEvent(context, L"DXR.BuildTLAS");
			// DEBUG_LOG_FMT(
			//	"[DxrRenderPass] TLAS Build requested: Frame={}, Instances={}, PreviousBuilt={}, PreviousCount={},
			// PendingLoads={}, AnimatedInstances={}, StableFrames={}, TopologyHashPrev={}, TopologyHashNow={}\n",
			//	frameIndex, tlasInstances.size(), tlas->IsBuilt(), tlas->GetInstanceCount(), pendingLoadsActive,
			//	hasAnimatedInstances, m_tlasStableFrameCount[frameIndex], m_lastTlasTopologyHash[frameIndex],
			// topologyHash
			//);
			tlas->Build(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), tlasInstances);
		}
		else
		{
			DxScopedGpuEvent refitEvent(context, L"DXR.RefitTLAS");
			// DEBUG_LOG_FMT(
			//	"[DxrRenderPass] TLAS Refit requested: Frame={}, Instances={}, StableFrames={}, AnimatedInstances={},
			// TopologyHash={}\n", 	frameIndex, tlasInstances.size(), m_tlasStableFrameCount[frameIndex],
			// hasAnimatedInstances, topologyHash
			//);
			tlas->Refit(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), tlasInstances);
		}

		m_lastTlasTopologyHash[frameIndex] = topologyHash;
		instanceData->tlasDescriptorIndex = tlas->GetSRVIndex();
		instanceData->tlasAddress = tlas->GetGPUAddress();
	}
}

void DxrRenderPass::CollectStaticMeshData(
	Scene* scene, ID3D12GraphicsCommandList4* cmdList, std::vector<DxTLASInstance>& tlasInstances, uint32_t frameIndex
)
{
	PixScopedCpuEvent event(L"DXR.CollectStaticMeshes");

	auto* meshStorage = scene->GetStorage<MeshComponent>();
	if (nullptr == meshStorage)
	{
		return;
	}

	auto& materialData = m_materialData[frameIndex];
	auto& geoTableData = m_geoTableData[frameIndex];
	auto& instanceData = m_instanceData[frameIndex];

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (false == meshComp.IsValid())
		{
			continue;
		}

		auto* meshRes = meshComp.GetMeshResource();
		if (nullptr == meshRes || !meshRes->IsReady() || nullptr == meshRes->GetVertexBuffer() ||
			nullptr == meshRes->GetIndexBuffer())
		{
			continue;
		}

		if (0 == meshRes->GetVertexBuffer()->GetGPUAddress() || 0 == meshRes->GetIndexBuffer()->GetGPUAddress())
		{
			continue;
		}

		auto* blas = meshRes->GetBLAS();
		if (nullptr == blas || false == blas->IsBuilt() || 0 == blas->GetGPUAddress())
		{
			continue;
		}

		uint32_t geoBaseIdx = static_cast<uint32_t>(geoTableData->syncBuffer.Size());
		bool	 disableTriangleCull = false;
		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto* matRes = meshComp.GetMaterial(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}
			disableTriangleCull |= (0 != (matRes->GetMaterialFlags() & MATERIAL_FLAG_DOUBLE_SIDED));

			uint32_t	matIdx = 0;
			const auto& matGuid = matRes->GetGuid();
			const bool	isRuntimeMaterial = IsNullGuid(matGuid);

			if (!isRuntimeMaterial && materialData->materialToIndex.contains(matGuid))
			{
				matIdx = materialData->materialToIndex[matGuid];
			}
			else
			{
				matIdx = static_cast<uint32_t>(materialData->syncBuffer.Size());
				MaterialGPUData gpuMat = {};
				gpuMat.albedo = matRes->GetAlbedo();
				gpuMat.roughness = matRes->GetRoughness();
				gpuMat.metallic = matRes->GetMetallic();
				gpuMat.emissive = matRes->GetEmissive();
				gpuMat.visibleEmissive = matRes->GetVisibleEmissive();
				gpuMat.shadingModel = static_cast<uint32_t>(matRes->GetShadingModelId());
				gpuMat.materialFlags = matRes->GetMaterialFlags();
				gpuMat.albedoTextureIdx = GetReadyTextureIndex(matRes, "ALBD");
				gpuMat.normalTextureIdx = GetReadyTextureIndex(matRes, "NRML");
				gpuMat.ormTextureIdx = GetReadyTextureIndex(matRes, "ORMS");
				gpuMat.emissiveTextureIdx = GetReadyTextureIndex(matRes, "EMSV");
				gpuMat.terrainSurfaceIdx = (0 != (matRes->GetMaterialFlags() & MATERIAL_FLAG_TERRAIN_SPLAT))
											   ? RegisterTerrainSurface(materialData.get(), matRes)
											   : ~0u;

				materialData->syncBuffer.Register(gpuMat);
				if (!isRuntimeMaterial)
				{
					materialData->materialToIndex[matGuid] = matIdx;
				}
			}

			geoTableData->syncBuffer.Register(
				{.vertexBase = 0, .indexBase = subMesh.indexOffset, .materialIdx = matIdx, .pad0 = 0}
			);
		}

		InstanceData inst = {};
		auto		 worldFloat = meshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		auto		 worldMat = DirectX::XMLoadFloat4x4(&worldFloat);
		DirectX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DirectX::XMStoreFloat4x4(&inst.worldInverse, DirectX::XMMatrixInverse(nullptr, worldMat));

		inst.vertexBufferIdx = meshRes->GetVertexBuffer()->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = meshComp.GetOwner().id;

		instanceData->syncBuffer.Register(inst);
		tlasInstances.push_back(
			{meshComp.GetGameObject(), blas,
			 disableTriangleCull ? D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
								 : D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		);
	}
}

void DxrRenderPass::CollectSkinnedMeshData(
	Scene*						 scene,
	ID3D12GraphicsCommandList4*	 cmdList,
	std::vector<DxTLASInstance>& tlasInstances,
	uint32_t					 frameIndex,
	bool&						 hasAnimatedInstances
)
{
	PixScopedCpuEvent event(L"DXR.CollectSkinnedMeshes");

	auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
	if (nullptr == skinnedMeshStorage)
	{
		return;
	}

	auto& materialData = m_materialData[frameIndex];
	auto& geoTableData = m_geoTableData[frameIndex];
	auto& instanceData = m_instanceData[frameIndex];

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

		auto* blas = skinnedMeshComp.GetBLAS(frameIndex);
		if (nullptr == blas)
		{
			// DEBUG_LOG_FMT(
			//	"[DxrRenderPass] Animated BLAS initial build: Frame={}, Mesh={}\n", frameIndex, meshRes->GetName()
			//);
			auto newBlas = std::make_unique<DxBLAS>();
			{
				PixScopedCommandListEvent blasEvent(cmdList, L"DXR.BuildAnimatedBLAS");
				newBlas->Build(
					m_device5.Get(), cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(),
					sizeof(EvAsset::Vertex), meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetIndexCount(),
					meshRes->GetSubMeshes(), true, meshRes->GetName() + "_AnimatedBLAS_" + std::to_string(frameIndex)
				);
			}

			const_cast<SkinnedMeshComponent&>(skinnedMeshComp).SetBLAS(frameIndex, std::move(newBlas));
			blas = skinnedMeshComp.GetBLAS(frameIndex);
		}
		else if (false == blas->IsBuilt())
		{
			// DEBUG_LOG_FMT(
			//	"[DxrRenderPass] Animated BLAS rebuild: Frame={}, Mesh={}\n", frameIndex, meshRes->GetName()
			//);
			PixScopedCommandListEvent blasEvent(cmdList, L"DXR.RebuildAnimatedBLAS");
			blas->Build(
				m_device5.Get(), cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(),
				sizeof(EvAsset::Vertex), meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetIndexCount(),
				meshRes->GetSubMeshes(), true, meshRes->GetName() + "_AnimatedBLAS_" + std::to_string(frameIndex)
			);
		}
		else
		{
			// DEBUG_LOG_FMT(
			//	"[DxrRenderPass] Animated BLAS refit: Frame={}, Mesh={}\n", frameIndex, meshRes->GetName()
			//);
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

		uint32_t geoBaseIdx = static_cast<uint32_t>(geoTableData->syncBuffer.Size());
		bool	 disableTriangleCull = false;
		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto* matRes = skinnedMeshComp.GetMaterialResource(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}
			disableTriangleCull |= (0 != (matRes->GetMaterialFlags() & MATERIAL_FLAG_DOUBLE_SIDED));

			uint32_t	matIdx = 0;
			const auto& matGuid = matRes->GetGuid();
			const bool	isRuntimeMaterial = IsNullGuid(matGuid);

			if (!isRuntimeMaterial && materialData->materialToIndex.contains(matGuid))
			{
				matIdx = materialData->materialToIndex[matGuid];
			}
			else
			{
				matIdx = static_cast<uint32_t>(materialData->syncBuffer.Size());
				MaterialGPUData gpuMat = {};
				gpuMat.albedo = matRes->GetAlbedo();
				gpuMat.roughness = matRes->GetRoughness();
				gpuMat.metallic = matRes->GetMetallic();
				gpuMat.emissive = matRes->GetEmissive();
				gpuMat.visibleEmissive = matRes->GetVisibleEmissive();
				gpuMat.shadingModel = static_cast<uint32_t>(matRes->GetShadingModelId());
				gpuMat.materialFlags = matRes->GetMaterialFlags();
				gpuMat.albedoTextureIdx = GetReadyTextureIndex(matRes, "ALBD");
				gpuMat.normalTextureIdx = GetReadyTextureIndex(matRes, "NRML");
				gpuMat.ormTextureIdx = GetReadyTextureIndex(matRes, "ORMS");
				gpuMat.emissiveTextureIdx = GetReadyTextureIndex(matRes, "EMSV");
				gpuMat.terrainSurfaceIdx = (0 != (matRes->GetMaterialFlags() & MATERIAL_FLAG_TERRAIN_SPLAT))
											   ? RegisterTerrainSurface(materialData.get(), matRes)
											   : ~0u;

				materialData->syncBuffer.Register(gpuMat);
				if (!isRuntimeMaterial)
				{
					materialData->materialToIndex[matGuid] = matIdx;
				}
			}

			geoTableData->syncBuffer.Register(
				{.vertexBase = 0, .indexBase = subMesh.indexOffset, .materialIdx = matIdx, .pad0 = 0}
			);
		}

		InstanceData inst = {};
		auto		 worldFloat = skinnedMeshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		auto		 worldMat = DirectX::XMLoadFloat4x4(&worldFloat);
		DirectX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DirectX::XMStoreFloat4x4(&inst.worldInverse, DirectX::XMMatrixInverse(nullptr, worldMat));

		inst.vertexBufferIdx = skinnedVB->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = skinnedMeshComp.GetOwner().id;

		instanceData->syncBuffer.Register(inst);
		tlasInstances.push_back(
			{skinnedMeshComp.GetGameObject(), blas,
			 disableTriangleCull ? D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
								 : D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		);
	}
}

void DxrRenderPass::CreateRaytracingPipeline()
{
	m_rtLitePipeline = std::make_unique<DxRtPipelineState>();
	m_rtLitePipeline->Create(m_device5.Get(), L"Resource/Shader/RaytracingPrimary.hlsl", 2);
	m_rtLiteShaderTable = std::make_unique<DxRtShaderTable>();
	m_rtLiteShaderTable->Build(m_device5.Get(), m_rtLitePipeline.get(), 1);

	m_rtPipeline = std::make_unique<DxRtPipelineState>();
	m_rtPipeline->Create(m_device5.Get(), L"Resource/Shader/RaytracingLibrary.hlsl", 4);
	m_rtShaderTable = std::make_unique<DxRtShaderTable>();
	m_rtShaderTable->Build(m_device5.Get(), m_rtPipeline.get(), 1);

	m_ptPipeline = std::make_unique<DxRtPipelineState>();
	m_ptPipeline->Create(m_device5.Get(), L"Resource/Shader/RaytracingLibraryPT.hlsl", 19);
	m_ptShaderTable = std::make_unique<DxRtShaderTable>();
	m_ptShaderTable->Build(m_device5.Get(), m_ptPipeline.get(), 1);

	DEBUG_LOG_FMT("[DxrRenderPass] Raytracing pipelines created\n");
}

void DxrRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"DxrRenderPass.Execute");

	if (!m_initialized || nullptr == scene || nullptr == renderContext)
	{
		return;
	}

	const uint32_t frameIndex = frame->GetFrameIndex();
	auto&		   instanceData = m_instanceData[frameIndex];
	auto&		   materialData = m_materialData[frameIndex];
	auto&		   geoTableData = m_geoTableData[frameIndex];
	auto&		   outputData = m_outputData[frameIndex];
	auto&		   tlas = m_tlas[frameIndex];

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
		ResetTemporalAccumulation();
	}
	if (input.GetInputDown(VK_F7))
	{
		m_useLiteRT = !m_useLiteRT;
		ResetTemporalAccumulation();
	}
	if (input.GetInputDown(VK_F8))
	{
		m_usePhysicalEmissionView = !m_usePhysicalEmissionView;
		ResetTemporalAccumulation();
	}

	{
		PixScopedCpuEvent setDataEvent(L"DXR.SetRenderContextData");
		renderContext->SetData(instanceData, 0);
		renderContext->SetData(materialData, 0);
		renderContext->SetData(geoTableData, 0);
		renderContext->SetData(outputData, 0);
	}

	auto				cameraData = renderContext->GetData<CameraRenderData>();
	const DX::XMFLOAT3* cameraPosition = cameraData ? &cameraData->cameraPosition : nullptr;

	auto&			 device = GLOBAL(DxDeviceGlobal);
	auto&			 context = *frame->GetMainContext();
	auto*			 uploadHeap = frame->GetUploadHeap();
	DxScopedGpuEvent passEvent(context, L"DxrRenderPass");

	PrepareRenderData(frame, scene, cameraPosition);

	{
		PixScopedCpuEvent syncCpuEvent(L"DXR.SyncRenderData");
		DxScopedGpuEvent syncEvent(context, L"DXR.SyncRenderData");
		instanceData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		materialData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		materialData->terrainSurfaceSyncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		geoTableData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
	}

	if (nullptr == tlas || false == tlas->IsBuilt())
	{
		return;
	}

	auto*							   cmdList = context.CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);
	auto& samplerHeap = GLOBAL(DxSamplerHeapGlobal);

	{
		PixScopedCpuEvent setupEvent(L"DXR.BindPipelineAndResources");

		ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap(), samplerHeap.GetHeap()};
		cmdList4->SetDescriptorHeaps(2, heaps);

		if (m_usePathTracing)
		{
			cmdList4->SetPipelineState1(m_ptPipeline->GetStateObject());
			cmdList4->SetComputeRootSignature(m_ptPipeline->GetGlobalRootSignature());
		}
		else if (m_useLiteRT)
		{
			cmdList4->SetPipelineState1(m_rtLitePipeline->GetStateObject());
			cmdList4->SetComputeRootSignature(m_rtLitePipeline->GetGlobalRootSignature());
		}
		else
		{
			cmdList4->SetPipelineState1(m_rtPipeline->GetStateObject());
			cmdList4->SetComputeRootSignature(m_rtPipeline->GetGlobalRootSignature());
		}

		cmdList4->SetComputeRootDescriptorTable(0, descHeap.GetGPUHandle(tlas->GetSRVIndex()));
		cmdList4->SetComputeRootDescriptorTable(1, descHeap.GetGPUHandle(outputData->outputTexture->GetUAVIndex(0)));
	}

	bool resetTemporalAccumulation = true;
	if (m_usePathTracing)
	{
		resetTemporalAccumulation = ShouldResetTemporalAccumulation(cameraData.get());
		if (resetTemporalAccumulation)
		{
			m_temporalAccumulationFrameCount = 0;
			m_temporalAccumulationReadIndex = 0;
			m_temporalAccumulationWriteIndex = 1;
		}
	}
	else
	{
		ResetTemporalAccumulation();
	}

	auto* temporalHistoryRead = m_ptAccumHistory[m_temporalAccumulationReadIndex].get();
	auto* temporalHistoryWrite = m_ptAccumHistory[m_temporalAccumulationWriteIndex].get();
	if (temporalHistoryRead && temporalHistoryRead->HasSRV())
	{
		cmdList4->SetComputeRootDescriptorTable(7, descHeap.GetGPUHandle(temporalHistoryRead->GetSRVIndex()));
	}
	if (temporalHistoryWrite && temporalHistoryWrite->HasUAV(0))
	{
		cmdList4->SetComputeRootDescriptorTable(8, descHeap.GetGPUHandle(temporalHistoryWrite->GetUAVIndex(0)));
	}

	struct TemporalAccumulationConstants
	{
		uint32_t frameCount;
		uint32_t enabled;
		uint32_t reset;
		uint32_t emissionViewMode;
	};
	TemporalAccumulationConstants temporalConstants = {
		m_temporalAccumulationFrameCount, m_usePathTracing ? 1u : 0u, resetTemporalAccumulation ? 1u : 0u,
		m_usePhysicalEmissionView ? 1u : 0u
	};
	cmdList4->SetComputeRoot32BitConstants(9, 4, &temporalConstants, 0);

	if (m_usePathTracing)
	{
		TransitionResourceIfNeeded(cmdList4.Get(), temporalHistoryRead, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		TransitionResourceIfNeeded(cmdList4.Get(), temporalHistoryWrite, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	if (nullptr != cameraData)
	{
		DX::XMFLOAT4X4 viewProjInvFloat;
		DirectX::XMStoreFloat4x4(&viewProjInvFloat, DirectX::XMMatrixTranspose(cameraData->viewProjInverse));
		cmdList4->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);
	}

	if (instanceData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			3, instanceData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (materialData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			4, materialData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (geoTableData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			5, geoTableData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (materialData->terrainSurfaceSyncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			6, materialData->terrainSurfaceSyncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}

	auto* shaderTable =
		m_usePathTracing ? m_ptShaderTable.get() : (m_useLiteRT ? m_rtLiteShaderTable.get() : m_rtShaderTable.get());
	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = shaderTable->GetRayGenRecord(),
		.MissShaderTable = shaderTable->GetMissTable(),
		.HitGroupTable = shaderTable->GetHitGroupTable(),
		.Width = outputData->outputTexture->GetWidth(),
		.Height = outputData->outputTexture->GetHeight(),
		.Depth = 1
	};

	{
		PixScopedCpuEvent dispatchCpuEvent(L"DXR.DispatchRays");
		DxScopedGpuEvent dispatchEvent(context, L"DXR.DispatchRays");
		cmdList4->DispatchRays(&desc);
	}

	if (m_usePathTracing)
	{
		TransitionResourceIfNeeded(
			cmdList4.Get(), temporalHistoryWrite, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);

		m_temporalAccumulationReadIndex = m_temporalAccumulationWriteIndex;
		m_temporalAccumulationWriteIndex = 1u - m_temporalAccumulationReadIndex;
		++m_temporalAccumulationFrameCount;
		m_temporalAccumulationResetPending = false;

		if (nullptr != cameraData)
		{
			m_hasTemporalCamera = true;
			m_lastTemporalCameraPosition = cameraData->cameraPosition;
			m_lastTemporalCameraDirection = cameraData->cameraDirection;
			m_lastTemporalFov = cameraData->fov;
			m_lastTemporalAspectRatio = cameraData->aspectRatio;
		}
	}
}
