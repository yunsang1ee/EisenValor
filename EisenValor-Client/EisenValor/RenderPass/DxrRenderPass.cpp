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
#include "DxRaytracingStateObjectBuilder.h"
#include "DxRootSignatureBuilder.h"
#include "DxShaderCompilerGlobal.h"
#include "DxUtils.h"
#include "Commonutils.h"
#include "MeshResource.h"
#include "MaterialResource.h"
#include "TextureResource.h"
#include "CameraRenderData.h"
#include "FrameRenderData.h"
#include "RaytracingCommon.h"

#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <string_view>

namespace
{
constexpr uint32_t kStableAssetMaterialIdSalt = 0x4d415441u;		  // MATA
constexpr uint32_t kStableRuntimeMaterialIdSalt = 0x4d415452u;		  // MATR
constexpr uint32_t kStableStaticAssetGeometryIdSalt = 0x47454f53u;	  // GEOS
constexpr uint32_t kStableSkinnedAssetGeometryIdSalt = 0x47454f4bu;	  // GEOK
constexpr uint32_t kStableStaticRuntimeGeometryIdSalt = 0x47455253u;  // GERS
constexpr uint32_t kStableSkinnedRuntimeGeometryIdSalt = 0x4745524bu; // GERK

constexpr uint32_t kRestirPrimaryHitUavRegister = 1;
constexpr uint32_t kRestirReservoirUavRegister = 2;
constexpr uint32_t kRestirMotionVectorUavRegister = 3;
constexpr uint32_t kRestirLinearDepthUavRegister = 4;

enum DxrRootParameter : uint32_t
{
	DxrRootScene = 0,
	DxrRootOutput,
	DxrRootCameraConstants,
	DxrRootInstanceBuffer,
	DxrRootMaterialBuffer,
	DxrRootGeoTableBuffer,
	DxrRootTerrainSurfaceBuffer,
	DxrRootFrameConstants,
	DxrRootRestirPrimaryHit,
	DxrRootRestirReservoir,
	DxrRootRestirCandidateConstants,
	DxrRootRestirMotionVector,
	DxrRootRestirLinearDepth,
	DxrRootRestirCandidateCameraConstants,
	DxrRootRestirEmissiveLights
};

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
	const float emissionLuminance =
		(0.2126f * std::max(emissive.x, 0.0f) + 0.7152f * std::max(emissive.y, 0.0f) +
		 0.0722f * std::max(emissive.z, 0.0f)) *
		std::max(emissive.w, 0.0f);
	const bool hasEmissiveMap = 0 != (material->GetMaterialFlags() & MATERIAL_FLAG_EMISSIVE_MAP);
	const float resolvedLuminance = hasEmissiveMap ? std::max(emissionLuminance, 1.0f) : emissionLuminance;
	return std::max(resolvedLuminance * static_cast<float>(triangleCount), 0.0001f);
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

} // namespace

DxrRenderPass::DxrRenderPass(uint32_t width, uint32_t height) : m_width(width), m_height(height)
{
	DirectX::XMStoreFloat4x4(&m_previousViewProj, DirectX::XMMatrixIdentity());
	GRAPHICS_LOG_FMT("[DxrRenderPass] Constructor: {}x{}\n", width, height);
}

void DxrRenderPass::Initialize()
{
	GRAPHICS_LOG_FMT("[DxrRenderPass] Initializing with resolution: {}x{}\n", m_width, m_height);

	auto& deviceG = GLOBAL(DxDeviceGlobal);
	auto* device = deviceG.GetDevice();
	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_device5)));

	for (int i = 0; i < 3; ++i)
	{
		m_tlas[i].Initialize(m_device5.Get());

		m_outputData[i].outputTexture = std::make_shared<DxTexture>();
	}
	m_restirCandidateData.Get().primaryHitBuffer = std::make_shared<DxBuffer>();
	m_restirCandidateData.Get().reservoirBuffer = std::make_shared<DxBuffer>();
	m_restirCandidateData.Get().motionVectorTexture = std::make_shared<DxTexture>();
	m_restirCandidateData.Get().linearDepthTexture = std::make_shared<DxTexture>();

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	GRAPHICS_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	m_tlas.Release();

	for (int i = 0; i < 3; ++i)
	{
		m_instanceData[i].Release();
		m_materialData[i].Release();
		m_geoTableData[i].Release();
		m_restirLightData[i].Release();
		m_outputData[i].Release();
	}
	m_restirCandidateData.Release();
	m_staticSceneData.Release();
	m_tlasInstancesScratch.clear();
	m_instanceIdLookupScratch.clear();
	m_device5.Reset();

	m_initialized = false;
	GRAPHICS_LOG_FMT("[DxrRenderPass] Released\n");
}

void DxrRenderPass::DeclareRenderData(RenderContext* renderContext)
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
	renderContext->DeclareAccess<RestirCandidateRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Write
	);
}

void DxrRenderPass::OnResize(uint32_t width, uint32_t height)
{
	GRAPHICS_LOG_FMT("[DxrRenderPass] Resizing: {}x{} -> {}x{}\n", m_width, m_height, width, height);

	m_width = width;
	m_height = height;
	m_hasPreviousViewProj = false;

	CreateRaytracingResources(m_width, m_height);
}

void DxrRenderPass::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	for (int i = 0; i < 3; ++i)
	{
		auto& outputTex = m_outputData[i].outputTexture;

		if (outputTex->HasUAV(0))
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			outputTex->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
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

	const uint32_t pixelCount = width * height;
	if (pixelCount > 0)
	{
		auto& candidateData = m_restirCandidateData.Get();
		auto& primaryHitBuffer = candidateData.primaryHitBuffer;
		auto& reservoirBuffer = candidateData.reservoirBuffer;
		auto& motionVectorTexture = candidateData.motionVectorTexture;
		auto& linearDepthTexture = candidateData.linearDepthTexture;

		if (!primaryHitBuffer)
		{
			primaryHitBuffer = std::make_shared<DxBuffer>();
		}
		if (!reservoirBuffer)
		{
			reservoirBuffer = std::make_shared<DxBuffer>();
		}
		if (!motionVectorTexture)
		{
			motionVectorTexture = std::make_shared<DxTexture>();
		}
		if (!linearDepthTexture)
		{
			linearDepthTexture = std::make_shared<DxTexture>();
		}

		if (primaryHitBuffer->HasUAV())
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			primaryHitBuffer->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
		}
		if (reservoirBuffer->HasUAV())
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			reservoirBuffer->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
		}
		if (motionVectorTexture->HasAnyUAV() || motionVectorTexture->HasSRV())
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			motionVectorTexture->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
		}
		if (linearDepthTexture->HasAnyUAV() || linearDepthTexture->HasSRV())
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			linearDepthTexture->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
		}

		primaryHitBuffer->Initialize(
			device.GetDevice(), static_cast<uint64_t>(pixelCount) * sizeof(RestirPrimaryHit), EBufferUsage::Structured,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			"DxrRenderPass_RestirCandidatePrimaryHit"
		);
		primaryHitBuffer->CreateSRV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirPrimaryHit));
		primaryHitBuffer->CreateUAV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirPrimaryHit));

		reservoirBuffer->Initialize(
			device.GetDevice(), static_cast<uint64_t>(pixelCount) * sizeof(RestirReservoir), EBufferUsage::Structured,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			"DxrRenderPass_RestirCandidateReservoir"
		);
		reservoirBuffer->CreateSRV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirReservoir));
		reservoirBuffer->CreateUAV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirReservoir));

		motionVectorTexture->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "DxrRenderPass_RestirMotionVector"
		);
		motionVectorTexture->CreateSRV(device.GetDevice(), descHeap);
		motionVectorTexture->CreateUAV(device.GetDevice(), descHeap, 0);

		linearDepthTexture->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "DxrRenderPass_RestirLinearDepth"
		);
		linearDepthTexture->CreateSRV(device.GetDevice(), descHeap);
		linearDepthTexture->CreateUAV(device.GetDevice(), descHeap, 0);
	}
}

ComPtr<ID3D12RootSignature> DxrRenderPass::BuildDxrGlobalRootSignature(
	bool enableRestirCandidateRoot, std::string_view name
) const
{
	DxRootSignatureBuilder builder;
	builder.AddDescriptorTable()
		.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)
		.AddDescriptorTable()
		.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
		.Add32BitConstants(16, 0)
		.AddSRV(1)
		.AddSRV(2)
		.AddSRV(3)
		.AddSRV(4)
		.Add32BitConstants(4, 2);

	if (enableRestirCandidateRoot)
	{
		builder.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirPrimaryHitUavRegister)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirReservoirUavRegister)
			.Add32BitConstants(8, 3)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirMotionVectorUavRegister)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirLinearDepthUavRegister)
			.Add32BitConstants(16, 4)
			.AddSRV(5);
	}

	return builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP)
		.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)
		.Build(m_device5.Get(), name);
}

std::unique_ptr<DxRtPipelineState> DxrRenderPass::BuildDxrPipeline(
	const std::wstring& shaderPath,
	uint32_t			maxRecursionDepth,
	uint32_t			maxPayloadSizeBytes,
	bool				enableRestirCandidateRoot
)
{
	auto& compiler = GLOBAL(DxShaderCompilerGlobal);

	const std::wstring shaderName = std::filesystem::path(shaderPath).stem().wstring();
	auto			   shaderBlob = compiler.CompileRTShader(shaderName, shaderPath, {});
	const std::string  pipelineName = Utils::WideToUtf8(shaderName.c_str());

	auto rootSignature = BuildDxrGlobalRootSignature(enableRestirCandidateRoot, pipelineName + "_GlobalRootSig");

	DxRaytracingStateObjectBuilder stateObjectBuilder;
	auto						   stateObject = stateObjectBuilder.AddDxilLibrary(shaderBlob.Get())
													 .AddHitGroup(L"HitGroup", L"ClosestHitMain")
													 .SetShaderConfig(maxPayloadSizeBytes, 2 * sizeof(float))
													 .SetPipelineConfig(maxRecursionDepth)
													 .SetGlobalRootSignature(rootSignature.Get())
													 .Build(m_device5.Get(), pipelineName + "_StateObject");

	auto pipeline = std::make_unique<DxRtPipelineState>();
	pipeline->SetStateObjects(rootSignature, stateObject);
	return pipeline;
}

std::unique_ptr<DxRtShaderTable> DxrRenderPass::BuildDxrShaderTable(
	const DxRtPipelineState* pipelineState, std::string_view name
) const
{
	DxRtShaderTableDesc desc;
	desc.rayGen = DxRtShaderRecordDesc(L"RayGenMain");
	desc.missShaders.emplace_back(L"MissMain");
	desc.hitGroups.emplace_back(L"HitGroup");

	auto shaderTable = std::make_unique<DxRtShaderTable>();
	shaderTable->Build(m_device5.Get(), pipelineState, desc, name);
	return shaderTable;
}

void DxrRenderPass::RegisterInstanceIdLookup(uint32_t ownerId, uint32_t instanceIndex)
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

void DxrRenderPass::PrepareRenderData(DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition)
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
	bool rebuildStaticCache = !staticSceneData.valid || staticSceneData.scene != scene ||
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
				{objectHandle.GetValue(), meshComponentHandle.GetValue(), instance.blas->GetGPUAddress(),
				 instance.flags}
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

		staticSceneData.scene = scene;
		staticSceneData.meshRevision = meshRevision;
		staticSceneData.meshComponentCount = staticMeshComponentCount;
		staticSceneData.valid = true;
		staticSceneData.complete = allStaticResourcesReady && !pendingLoadsActive;
	}
	else
	{
		PixScopedCpuEvent restoreStaticCacheEvent(L"DXR.RestoreStaticSceneCache");
		instanceData->syncBuffer.Assign(staticSceneData.instances);
		materialData->syncBuffer.Assign(staticSceneData.materials);
		materialData->terrainSurfaceSyncBuffer.Assign(staticSceneData.terrainSurfaces);
		geoTableData->syncBuffer.Assign(staticSceneData.geometry);
		restirLightData->emissiveLightSync.Assign(staticSceneData.emissiveLights);
		restirLightData->emissiveLightCount = static_cast<uint32_t>(staticSceneData.emissiveLights.size());
		restirLightData->emissiveLightWeightSum = staticSceneData.emissiveLightWeightSum;
		materialData->materialToIndex = staticSceneData.materialToIndex;
		m_instanceIdLookupScratch = staticSceneData.instanceIdLookup;
	}
	staticSceneData.pendingLoadsActive = pendingLoadsActive;

	{
		PixScopedCpuEvent movableEvent(L"DXR.CollectMovableMeshData");
		bool			  allMovableResourcesReady = true;
		CollectMeshData(scene, m_tlasInstancesScratch, frameIndex, RenderMobility::Movable, allMovableResourcesReady);
	}
	{
		PixScopedCpuEvent skinnedEvent(L"DXR.CollectSkinnedMeshData");
		CollectSkinnedMeshData(scene, cmdList4.Get(), m_tlasInstancesScratch, frameIndex, hasAnimatedInstances);
	}

	if (nullptr != tlas && false == m_tlasInstancesScratch.empty())
	{
		uint64_t topologyHash = staticSceneData.topologyHash;
		uint64_t transformHash = staticSceneData.transformHash;
		for (size_t i = staticSceneData.tlasInstances.size(); i < m_tlasInstancesScratch.size(); ++i)
		{
			HashTlasInstance(topologyHash, transformHash, m_tlasInstancesScratch[i]);
		}

		const uint32_t currentInstanceCount = static_cast<uint32_t>(m_tlasInstancesScratch.size());
		const bool	   topologyChanged = !tlas->IsBuilt() || tlas->GetInstanceCount() != currentInstanceCount ||
										 tlasFrame.lastInstanceCount != currentInstanceCount ||
										 tlasFrame.lastTopologyHash != topologyHash;
		const bool	   transformsChanged = tlasFrame.lastTransformHash != transformHash;

		if (topologyChanged)
		{
			DxScopedGpuEvent buildEvent(context, L"DXR.BuildTLAS");
			tlas->Build(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), m_tlasInstancesScratch);
		}
		else if (transformsChanged || hasAnimatedInstances)
		{
			DxScopedGpuEvent refitEvent(context, L"DXR.RefitTLAS");
			tlas->Refit(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), m_tlasInstancesScratch);
		}

		tlasFrame.lastInstanceCount = currentInstanceCount;
		tlasFrame.lastTopologyHash = topologyHash;
		tlasFrame.lastTransformHash = transformHash;
		instanceData->tlasDescriptorIndex = tlas->GetSRVIndex();
		instanceData->tlasAddress = tlas->GetGPUAddress();
	}
}

void DxrRenderPass::CollectMeshData(
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

	auto* materialData = &m_materialData[frameIndex];
	auto* geoTableData = &m_geoTableData[frameIndex];
	auto* instanceData = &m_instanceData[frameIndex];
	auto* restirLightData = &m_restirLightData[frameIndex];

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

			uint32_t	   matIdx = 0;
			const auto&	   matGuid = matRes->GetGuid();
			const bool	   isRuntimeMaterial = matGuid.IsNull();
			const uint32_t stableMaterialId =
				isRuntimeMaterial ? MakeStableRuntimeId(kStableRuntimeMaterialIdSalt, ownerId, subMesh.materialSlot)
								  : MakeStableAssetId(kStableAssetMaterialIdSalt, matGuid);
			const auto&	   meshGuid = meshRes->GetGuid();
			const bool	   isRuntimeMesh = meshGuid.IsNull();
			const uint32_t stableGeometryId =
				isRuntimeMesh ? MakeStableRuntimeId(kStableStaticRuntimeGeometryIdSalt, ownerId, subMeshIndex)
							  : MakeStableAssetId(kStableStaticAssetGeometryIdSalt, meshGuid, subMeshIndex);

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
											   ? RegisterTerrainSurface(materialData, matRes)
											   : ~0u;
				gpuMat.stableMaterialId = stableMaterialId;

				materialData->syncBuffer.Register(gpuMat);
				if (!isRuntimeMaterial)
				{
					materialData->materialToIndex[matGuid] = matIdx;
				}
			}

			const bool	   isEmissiveLight = IsRestirEmissiveLightMaterial(matRes) && subMesh.indexCount >= 3;
			const uint32_t triangleCount = subMesh.indexCount / 3u;
			const uint32_t emissiveEntryIdx =
				isEmissiveLight ? static_cast<uint32_t>(restirLightData->emissiveLightSync.Size()) : ~0u;

			geoTableData->syncBuffer.Register(
				{.vertexBase = 0,
				 .indexBase = subMesh.indexOffset,
				 .materialIdx = matIdx,
				 .stableGeometryId = stableGeometryId,
				 .emissiveEntryIdx = emissiveEntryIdx}
			);
			if (isEmissiveLight)
			{
				const float selectionWeight = EstimateRestirEmissiveLightSelectionWeight(matRes, triangleCount);
				restirLightData->emissiveLightSync.Register(
					{.instanceIndex = mappedInstanceIndex,
					 .geometryIndex = subMeshIndex,
					 .triangleCount = triangleCount,
					 .selectionWeight = selectionWeight}
				);
				restirLightData->emissiveLightWeightSum += selectionWeight;
			}
		}

		InstanceData inst = {};
		auto		 worldFloat = meshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		auto		 worldMat = DirectX::XMLoadFloat4x4(&worldFloat);
		DirectX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DirectX::XMStoreFloat4x4(&inst.worldInverse, DirectX::XMMatrixInverse(nullptr, worldMat));

		inst.vertexBufferIdx = meshRes->GetVertexBuffer()->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = ownerId;
		inst.generation = meshComp.GetOwner().generation;

		instanceData->syncBuffer.Register(inst);
		RegisterInstanceIdLookup(ownerId, mappedInstanceIndex);
		tlasInstances.push_back(
			{meshComp.GetGameObject(), blas,
			 disableTriangleCull ? D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
								 : D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		);
	}
}

uint32_t DxrRenderPass::RegisterTerrainSurface(MaterialRenderData* materialData, MaterialResource* material) const
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

	auto* materialData = &m_materialData[frameIndex];
	auto* geoTableData = &m_geoTableData[frameIndex];
	auto* instanceData = &m_instanceData[frameIndex];
	auto* restirLightData = &m_restirLightData[frameIndex];

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
			// GRAPHICS_LOG_FMT(
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
			// GRAPHICS_LOG_FMT(
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
			// GRAPHICS_LOG_FMT(
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

			uint32_t	   matIdx = 0;
			const auto&	   matGuid = matRes->GetGuid();
			const bool	   isRuntimeMaterial = matGuid.IsNull();
			const uint32_t stableMaterialId =
				isRuntimeMaterial ? MakeStableRuntimeId(kStableRuntimeMaterialIdSalt, ownerId, subMesh.materialSlot)
								  : MakeStableAssetId(kStableAssetMaterialIdSalt, matGuid);
			const auto&	   meshGuid = meshRes->GetGuid();
			const bool	   isRuntimeMesh = meshGuid.IsNull();
			const uint32_t stableGeometryId =
				isRuntimeMesh ? MakeStableRuntimeId(kStableSkinnedRuntimeGeometryIdSalt, ownerId, subMeshIndex)
							  : MakeStableAssetId(kStableSkinnedAssetGeometryIdSalt, meshGuid, subMeshIndex);

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
											   ? RegisterTerrainSurface(materialData, matRes)
											   : ~0u;
				gpuMat.stableMaterialId = stableMaterialId;

				materialData->syncBuffer.Register(gpuMat);
				if (!isRuntimeMaterial)
				{
					materialData->materialToIndex[matGuid] = matIdx;
				}
			}

			const bool	   isEmissiveLight = IsRestirEmissiveLightMaterial(matRes) && subMesh.indexCount >= 3;
			const uint32_t triangleCount = subMesh.indexCount / 3u;
			const uint32_t emissiveEntryIdx =
				isEmissiveLight ? static_cast<uint32_t>(restirLightData->emissiveLightSync.Size()) : ~0u;

			geoTableData->syncBuffer.Register(
				{.vertexBase = 0,
				 .indexBase = subMesh.indexOffset,
				 .materialIdx = matIdx,
				 .stableGeometryId = stableGeometryId,
				 .emissiveEntryIdx = emissiveEntryIdx}
			);
			if (isEmissiveLight)
			{
				const float selectionWeight = EstimateRestirEmissiveLightSelectionWeight(matRes, triangleCount);
				restirLightData->emissiveLightSync.Register(
					{.instanceIndex = mappedInstanceIndex,
					 .geometryIndex = subMeshIndex,
					 .triangleCount = triangleCount,
					 .selectionWeight = selectionWeight}
				);
				restirLightData->emissiveLightWeightSum += selectionWeight;
			}
		}

		InstanceData inst = {};
		auto		 worldFloat = skinnedMeshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		auto		 worldMat = DirectX::XMLoadFloat4x4(&worldFloat);
		DirectX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DirectX::XMStoreFloat4x4(&inst.worldInverse, DirectX::XMMatrixInverse(nullptr, worldMat));

		inst.vertexBufferIdx = skinnedVB->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = ownerId;
		inst.generation = skinnedMeshComp.GetOwner().generation;

		instanceData->syncBuffer.Register(inst);
		RegisterInstanceIdLookup(ownerId, mappedInstanceIndex);
		tlasInstances.push_back(
			{skinnedMeshComp.GetGameObject(), blas,
			 disableTriangleCull ? D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
								 : D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		);
	}
}

void DxrRenderPass::CreateRaytracingPipeline()
{
	m_rtPipeline = BuildDxrPipeline(L"Resource/Shader/RaytracingLibrary.hlsl", 4, 16, false);
	m_rtShaderTable = BuildDxrShaderTable(m_rtPipeline.get(), "RaytracingLibrary_ShaderTable");

	m_ptPipeline = BuildDxrPipeline(L"Resource/Shader/RaytracingLibraryPT.hlsl", 19, 16, false);
	m_ptShaderTable = BuildDxrShaderTable(m_ptPipeline.get(), "RaytracingLibraryPT_ShaderTable");

	// payload * MAX_RECURSION_DEPTH -> reserve per-lane stack increased -> occupancy reduced -> lower performance
	m_restirCandidatePipeline = BuildDxrPipeline(L"Resource/Shader/RaytracingRestirPT.hlsl", 7, 68, true);
	m_restirCandidateShaderTable =
		BuildDxrShaderTable(m_restirCandidatePipeline.get(), "RaytracingRestirPT_ShaderTable");

	GRAPHICS_LOG_FMT("[DxrRenderPass] Raytracing pipelines created\n");
}

void DxrRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"DxrRenderPass.Execute");

	if (!m_initialized || nullptr == scene || nullptr == renderContext)
	{
		return;
	}

	const uint32_t frameIndex = frame->GetFrameIndex();
	m_instanceData.BeginFrame(frameIndex);
	m_materialData.BeginFrame(frameIndex);
	m_geoTableData.BeginFrame(frameIndex);
	m_restirLightData.BeginFrame(frameIndex);
	m_outputData.BeginFrame(frameIndex);
	m_tlas.BeginFrame(frameIndex);
	m_restirCandidateData.BeginFrame(frameIndex);
	m_staticSceneData.BeginFrame(frameIndex);
	auto* instanceData = &m_instanceData.GetCurrent();
	auto* materialData = &m_materialData.GetCurrent();
	auto* geoTableData = &m_geoTableData.GetCurrent();
	auto* restirLightData = &m_restirLightData.GetCurrent();
	auto& outputData = m_outputData.GetCurrent();
	auto* restirCandidateData = &m_restirCandidateData.Get();
	auto* tlas = m_tlas.GetCurrent().Get();
	outputData.bypassToneMap = false;
	restirCandidateData->validThisFrame = false;
	restirCandidateData->frameIndex = frameIndex;

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
	}
	if (input.GetInputDown(VK_F7))
	{
		m_useRestirPT = !m_useRestirPT;
	}
	if (input.GetInputDown(VK_F8))
	{
		m_usePhysicalEmissionView = !m_usePhysicalEmissionView;
	}
	if (input.GetInputDown(VK_F9))
	{
		m_useDayEnvironment = !m_useDayEnvironment;
	}
	const bool restirCandidateMode = m_useRestirPT;
	auto*	   restirPrimaryHitBuffer = restirCandidateData ? restirCandidateData->primaryHitBuffer.get() : nullptr;
	auto*	   restirReservoirBuffer = restirCandidateData ? restirCandidateData->reservoirBuffer.get() : nullptr;
	auto* restirMotionVectorTexture = restirCandidateData ? restirCandidateData->motionVectorTexture.get() : nullptr;
	auto* restirLinearDepthTexture = restirCandidateData ? restirCandidateData->linearDepthTexture.get() : nullptr;
	const bool restirCandidateResourcesReady =
		restirPrimaryHitBuffer && restirPrimaryHitBuffer->HasUAV() && restirPrimaryHitBuffer->HasSRV() &&
		restirReservoirBuffer && restirReservoirBuffer->HasUAV() && restirReservoirBuffer->HasSRV() &&
		restirMotionVectorTexture && restirMotionVectorTexture->HasUAV(0) && restirMotionVectorTexture->HasSRV() &&
		restirLinearDepthTexture && restirLinearDepthTexture->HasUAV(0) && restirLinearDepthTexture->HasSRV();
	const bool restirCandidateEnabled = restirCandidateMode && restirCandidateResourcesReady;

	{
		PixScopedCpuEvent setDataEvent(L"DXR.SetRenderContextData");
		renderContext->Set(m_instanceData);
		renderContext->Set(m_materialData);
		renderContext->Set(m_geoTableData);
		renderContext->Set(m_restirLightData);
		renderContext->Set(m_tlas);
		renderContext->Set(m_outputData);
		renderContext->Set(m_restirCandidateData);
	}

	auto* outputTexture = outputData.outputTexture.get();
	if (nullptr == outputTexture || !outputTexture->HasUAV(0))
	{
		return;
	}

	auto*				cameraData = renderContext->Get<CameraRenderData>();
	const DX::XMFLOAT3* cameraPosition = cameraData ? &cameraData->cameraPosition : nullptr;
	DirectX::XMMATRIX	currentViewProj = DirectX::XMMatrixIdentity();
	if (nullptr != cameraData)
	{
		currentViewProj = DirectX::XMMatrixMultiply(cameraData->viewMatrix, cameraData->projectionMatrix);
	}
	DirectX::XMMATRIX previousViewProj =
		m_hasPreviousViewProj ? DirectX::XMLoadFloat4x4(&m_previousViewProj) : currentViewProj;

	auto&			 device = GLOBAL(DxDeviceGlobal);
	auto&			 context = *frame->GetMainContext();
	auto*			 uploadHeap = frame->GetUploadHeap();
	DxScopedGpuEvent passEvent(context, L"DxrRenderPass");

	PrepareRenderData(frame, scene, cameraPosition);
	restirLightData->emissiveLightCount = static_cast<uint32_t>(restirLightData->emissiveLightSync.Size());
	if (0 == restirLightData->emissiveLightCount)
	{
		restirLightData->emissiveLightSync.Register(RestirEmissiveLightData{});
	}

	{
		PixScopedCpuEvent syncCpuEvent(L"DXR.SyncRenderData");
		DxScopedGpuEvent  syncEvent(context, L"DXR.SyncRenderData");
		instanceData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		materialData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		materialData->terrainSurfaceSyncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		geoTableData->syncBuffer.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		restirLightData->emissiveLightSync.SyncToGPU(device.GetDevice(), context, *uploadHeap);

		if (restirCandidateMode && !m_instanceIdLookupScratch.empty())
		{
			auto& lookupSync = instanceData->idToInstanceIndexSync;
			lookupSync.Assign(m_instanceIdLookupScratch);
			lookupSync.SyncToGPU(device.GetDevice(), context, *uploadHeap);
		}
		m_staticSceneData.ClearDirty();
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

		if (restirCandidateMode)
		{
			cmdList4->SetPipelineState1(m_restirCandidatePipeline->GetStateObject());
			cmdList4->SetComputeRootSignature(m_restirCandidatePipeline->GetGlobalRootSignature());
		}
		else if (m_usePathTracing)
		{
			cmdList4->SetPipelineState1(m_ptPipeline->GetStateObject());
			cmdList4->SetComputeRootSignature(m_ptPipeline->GetGlobalRootSignature());
		}
		else
		{
			cmdList4->SetPipelineState1(m_rtPipeline->GetStateObject());
			cmdList4->SetComputeRootSignature(m_rtPipeline->GetGlobalRootSignature());
		}

		cmdList4->SetComputeRootDescriptorTable(DxrRootScene, descHeap.GetGPUHandle(tlas->GetSRVIndex()));
		cmdList4->SetComputeRootDescriptorTable(DxrRootOutput, descHeap.GetGPUHandle(outputTexture->GetUAVIndex(0)));
	}

	struct RaytracingFrameConstants
	{
		uint32_t frameSeed;
		uint32_t emissionViewMode;
		uint32_t environmentMode;
		uint32_t pad0;
	};
	RaytracingFrameConstants frameConstants = {
		m_raytracingFrameSeed++, m_usePhysicalEmissionView ? 1u : 0u, m_useDayEnvironment ? 1u : 0u, 0u
	};
	cmdList4->SetComputeRoot32BitConstants(DxrRootFrameConstants, 4, &frameConstants, 0);

	if (nullptr != cameraData)
	{
		DX::XMFLOAT4X4 viewProjInvFloat;
		DirectX::XMStoreFloat4x4(&viewProjInvFloat, DirectX::XMMatrixTranspose(cameraData->viewProjInverse));
		cmdList4->SetComputeRoot32BitConstants(DxrRootCameraConstants, 16, &viewProjInvFloat, 0);
	}

	if (instanceData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootInstanceBuffer, instanceData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (materialData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootMaterialBuffer, materialData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (geoTableData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootGeoTableBuffer, geoTableData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (materialData->terrainSurfaceSyncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootTerrainSurfaceBuffer,
			materialData->terrainSurfaceSyncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}

	if (restirCandidateMode)
	{
		if (restirCandidateResourcesReady)
		{
			DxUtils::TransitionResourceIfNeeded(
				cmdList4.Get(), restirPrimaryHitBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
			cmdList4->SetComputeRootDescriptorTable(
				DxrRootRestirPrimaryHit, descHeap.GetGPUHandle(restirPrimaryHitBuffer->GetUAVHandle().GetIndex())
			);
			DxUtils::TransitionResourceIfNeeded(
				cmdList4.Get(), restirReservoirBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
			cmdList4->SetComputeRootDescriptorTable(
				DxrRootRestirReservoir, descHeap.GetGPUHandle(restirReservoirBuffer->GetUAVHandle().GetIndex())
			);
			DxUtils::TransitionResourceIfNeeded(
				cmdList4.Get(), restirMotionVectorTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
			cmdList4->SetComputeRootDescriptorTable(
				DxrRootRestirMotionVector, descHeap.GetGPUHandle(restirMotionVectorTexture->GetUAVIndex(0))
			);
			DxUtils::TransitionResourceIfNeeded(
				cmdList4.Get(), restirLinearDepthTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
			cmdList4->SetComputeRootDescriptorTable(
				DxrRootRestirLinearDepth, descHeap.GetGPUHandle(restirLinearDepthTexture->GetUAVIndex(0))
			);
			if (auto* emissiveLightBuffer = restirLightData->emissiveLightSync.GetBuffer())
			{
				cmdList4->SetComputeRootShaderResourceView(
					DxrRootRestirEmissiveLights, emissiveLightBuffer->GetResource()->GetGPUVirtualAddress()
				);
			}
		}

		struct RestirCandidateConstants
		{
			uint32_t enabled;
			uint32_t screenWidth;
			uint32_t screenHeight;
			float	 cameraNearZ;
			uint32_t emissiveLightCount;
			float	 emissiveLightWeightSum;
			uint32_t pad1;
			uint32_t pad2;
		};
		RestirCandidateConstants restirConstants = {
			restirCandidateEnabled ? 1u : 0u,
			m_width,
			m_height,
			cameraData ? cameraData->nearZ : 0.1f,
			restirLightData->emissiveLightCount,
			restirLightData->emissiveLightWeightSum,
			0u,
			0u
		};
		cmdList4->SetComputeRoot32BitConstants(DxrRootRestirCandidateConstants, 8, &restirConstants, 0);

		struct RestirCandidateCameraConstants
		{
			DX::XMFLOAT4X4 previousViewProj;
		};
		RestirCandidateCameraConstants restirCameraConstants;
		DirectX::XMStoreFloat4x4(&restirCameraConstants.previousViewProj, DirectX::XMMatrixTranspose(previousViewProj));
		cmdList4->SetComputeRoot32BitConstants(DxrRootRestirCandidateCameraConstants, 16, &restirCameraConstants, 0);
	}

	auto*					 shaderTable = restirCandidateMode ? m_restirCandidateShaderTable.get()
										   : m_usePathTracing  ? m_ptShaderTable.get()
															   : m_rtShaderTable.get();
	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = shaderTable->GetRayGenRecord(),
		.MissShaderTable = shaderTable->GetMissTable(),
		.HitGroupTable = shaderTable->GetHitGroupTable(),
		.Width = outputTexture->GetWidth(),
		.Height = outputTexture->GetHeight(),
		.Depth = 1
	};

	{
		PixScopedCpuEvent dispatchCpuEvent(L"DXR.DispatchRays");
		DxScopedGpuEvent  dispatchEvent(context, L"DXR.DispatchRays");
		cmdList4->DispatchRays(&desc);
	}

	if (restirCandidateEnabled)
	{
		D3D12_RESOURCE_BARRIER restirBarriers[] = {
			DxUtils::CreateUAVBarrier(restirPrimaryHitBuffer->GetResource()),
			DxUtils::CreateUAVBarrier(restirReservoirBuffer->GetResource()),
			DxUtils::CreateUAVBarrier(restirMotionVectorTexture->GetResource()),
			DxUtils::CreateUAVBarrier(restirLinearDepthTexture->GetResource())
		};
		cmdList4->ResourceBarrier(4, restirBarriers);
		DxUtils::TransitionResourceIfNeeded(
			cmdList4.Get(), restirPrimaryHitBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		DxUtils::TransitionResourceIfNeeded(
			cmdList4.Get(), restirReservoirBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		DxUtils::TransitionResourceIfNeeded(
			cmdList4.Get(), restirMotionVectorTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		DxUtils::TransitionResourceIfNeeded(
			cmdList4.Get(), restirLinearDepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		restirCandidateData->validThisFrame = true;
		restirCandidateData->frameIndex = frameIndex;
	}

	if (nullptr != cameraData)
	{
		DirectX::XMStoreFloat4x4(&m_previousViewProj, currentViewProj);
		m_hasPreviousViewProj = true;
	}
}
