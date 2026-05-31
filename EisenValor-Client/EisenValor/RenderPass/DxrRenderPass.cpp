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
#include "MeshResource.h"
#include "MaterialResource.h"
#include "TextureResource.h"
#include "CameraRenderData.h"
#include "FrameRenderData.h"
#include "RaytracingCommon.h"

#include <unordered_map>
#include <algorithm>
#include <string_view>

namespace
{
constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

std::wstring ExtractShaderName(const std::wstring& shaderPath)
{
	std::wstring shaderName = shaderPath;
	const size_t lastSlash = shaderName.find_last_of(L"/\\");
	if (lastSlash != std::wstring::npos)
	{
		shaderName = shaderName.substr(lastSlash + 1);
	}

	const size_t dotPos = shaderName.find_last_of(L'.');
	if (dotPos != std::wstring::npos)
	{
		shaderName.resize(dotPos);
	}

	return shaderName;
}

std::string ToNarrowString(std::wstring_view text)
{
	std::string result;
	result.reserve(text.size());

	for (wchar_t ch : text)
	{
		result.push_back(static_cast<char>(ch));
	}

	return result;
}

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
		m_tlas[i] = std::make_unique<DxTLAS>();
		m_tlas[i]->Initialize(m_device5.Get());

		m_outputData[i].outputTexture = std::make_shared<DxTexture>();
	}
	m_restirPrimaryHitCurrentData.Get().primaryHitBuffer = std::make_shared<DxBuffer>();
	m_restirReservoirInitialData.Get().reservoirBuffer = std::make_shared<DxBuffer>();

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	GRAPHICS_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	for (int i = 0; i < 3; ++i)
	{
		m_tlas[i].reset();
		m_instanceData[i].Release();
		m_materialData[i].Release();
		m_geoTableData[i].Release();
		m_outputData[i].Release();
	}
	m_restirPrimaryHitCurrentData.Release();
	m_restirReservoirInitialData.Release();
	m_restirFinalReservoirData.Release();
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
	renderContext->DeclareAccess<RaytracingOutputRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<RestirPrimaryHitCurrentRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<RestirReservoirInitialRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Write
	);
	renderContext->DeclareAccess<RestirFinalReservoirRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Write
	);
}

void DxrRenderPass::OnResize(uint32_t width, uint32_t height)
{
	GRAPHICS_LOG_FMT("[DxrRenderPass] Resizing: {}x{} -> {}x{}\n", m_width, m_height, width, height);

	m_width = width;
	m_height = height;

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
		auto& primaryHitBuffer = m_restirPrimaryHitCurrentData.Get().primaryHitBuffer;
		auto& reservoirBuffer = m_restirReservoirInitialData.Get().reservoirBuffer;

		if (!primaryHitBuffer)
		{
			primaryHitBuffer = std::make_shared<DxBuffer>();
		}
		if (!reservoirBuffer)
		{
			reservoirBuffer = std::make_shared<DxBuffer>();
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

		primaryHitBuffer->Initialize(
			device.GetDevice(), static_cast<uint64_t>(pixelCount) * sizeof(RestirPrimaryHit), EBufferUsage::Structured,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			"DxrRenderPass_RestirPrimaryHitCurrent"
		);
		primaryHitBuffer->CreateSRV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirPrimaryHit));
		primaryHitBuffer->CreateUAV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirPrimaryHit));

		reservoirBuffer->Initialize(
			device.GetDevice(), static_cast<uint64_t>(pixelCount) * sizeof(RestirReservoir), EBufferUsage::Structured,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			"DxrRenderPass_RestirReservoirInitial"
		);
		reservoirBuffer->CreateSRV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirReservoir));
		reservoirBuffer->CreateUAV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirReservoir));
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
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3)
			.Add32BitConstants(4, 3);
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

	const std::wstring shaderName = ExtractShaderName(shaderPath);
	auto			   shaderBlob = compiler.CompileRTShader(shaderName, shaderPath, {});
	const std::string  pipelineName = ToNarrowString(shaderName);

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

void DxrRenderPass::PrepareRenderData(DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition)
{
	PixScopedCpuEvent cpuEvent(L"DXR.PrepareRenderData");
	auto&			  context = *frame->GetMainContext();

	const uint32_t frameIndex = frame->GetFrameIndex();
	auto*		   instanceData = &m_instanceData[frameIndex];
	auto*		   materialData = &m_materialData[frameIndex];
	auto*		   geoTableData = &m_geoTableData[frameIndex];
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
			// GRAPHICS_LOG_FMT(
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
			// GRAPHICS_LOG_FMT(
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

	auto* materialData = &m_materialData[frameIndex];
	auto* geoTableData = &m_geoTableData[frameIndex];
	auto* instanceData = &m_instanceData[frameIndex];

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
											   ? RegisterTerrainSurface(materialData, matRes)
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
											   ? RegisterTerrainSurface(materialData, matRes)
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
	m_rtPipeline = BuildDxrPipeline(L"Resource/Shader/RaytracingLibrary.hlsl", 4, 80, false);
	m_rtShaderTable = BuildDxrShaderTable(m_rtPipeline.get(), "RaytracingLibrary_ShaderTable");

	m_ptPipeline = BuildDxrPipeline(L"Resource/Shader/RaytracingLibraryPT.hlsl", 19, 80, false);
	m_ptShaderTable = BuildDxrShaderTable(m_ptPipeline.get(), "RaytracingLibraryPT_ShaderTable");

	m_restirCandidatePipeline = BuildDxrPipeline(L"Resource/Shader/RaytracingRestirPT.hlsl", 19, 80, true);
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
	m_outputData.BeginFrame(frameIndex);
	m_restirPrimaryHitCurrentData.BeginFrame(frameIndex);
	m_restirReservoirInitialData.BeginFrame(frameIndex);
	m_restirFinalReservoirData.BeginFrame(frameIndex);
	auto* instanceData = &m_instanceData.GetCurrent();
	auto* materialData = &m_materialData.GetCurrent();
	auto* geoTableData = &m_geoTableData.GetCurrent();
	auto& outputData = m_outputData.GetCurrent();
	auto* restirPrimaryHitData = &m_restirPrimaryHitCurrentData.Get();
	auto* restirReservoirData = &m_restirReservoirInitialData.Get();
	auto* restirFinalReservoirData = &m_restirFinalReservoirData.Get();
	auto& tlas = m_tlas[frameIndex];
	outputData.bypassToneMap = false;

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
	auto*	   restirPrimaryHitBuffer = restirPrimaryHitData ? restirPrimaryHitData->primaryHitBuffer.get() : nullptr;
	auto*	   restirReservoirBuffer = restirReservoirData ? restirReservoirData->reservoirBuffer.get() : nullptr;
	const bool restirCandidateResourcesReady = restirPrimaryHitBuffer && restirPrimaryHitBuffer->HasUAV() &&
											   restirPrimaryHitBuffer->HasSRV() && restirReservoirBuffer &&
											   restirReservoirBuffer->HasUAV() && restirReservoirBuffer->HasSRV();
	const bool restirCandidateEnabled = restirCandidateMode && restirCandidateResourcesReady;
	restirFinalReservoirData->Release();

	{
		PixScopedCpuEvent setDataEvent(L"DXR.SetRenderContextData");
		renderContext->Set(m_instanceData);
		renderContext->Set(m_materialData);
		renderContext->Set(m_geoTableData);
		renderContext->Set(m_outputData);
		renderContext->Set(m_restirPrimaryHitCurrentData);
		renderContext->Set(m_restirReservoirInitialData);
		renderContext->Set(m_restirFinalReservoirData);
	}

	auto* outputTexture = outputData.outputTexture.get();
	if (nullptr == outputTexture || !outputTexture->HasUAV(0))
	{
		return;
	}

	auto*				cameraData = renderContext->Get<CameraRenderData>();
	const DX::XMFLOAT3* cameraPosition = cameraData ? &cameraData->cameraPosition : nullptr;

	auto&			 device = GLOBAL(DxDeviceGlobal);
	auto&			 context = *frame->GetMainContext();
	auto*			 uploadHeap = frame->GetUploadHeap();
	DxScopedGpuEvent passEvent(context, L"DxrRenderPass");

	PrepareRenderData(frame, scene, cameraPosition);

	{
		PixScopedCpuEvent syncCpuEvent(L"DXR.SyncRenderData");
		DxScopedGpuEvent  syncEvent(context, L"DXR.SyncRenderData");
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

		cmdList4->SetComputeRootDescriptorTable(0, descHeap.GetGPUHandle(tlas->GetSRVIndex()));
		cmdList4->SetComputeRootDescriptorTable(1, descHeap.GetGPUHandle(outputTexture->GetUAVIndex(0)));
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
	cmdList4->SetComputeRoot32BitConstants(7, 4, &frameConstants, 0);

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

	if (restirCandidateMode)
	{
		if (restirCandidateResourcesReady)
		{
			TransitionResourceIfNeeded(cmdList4.Get(), restirPrimaryHitBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList4->SetComputeRootDescriptorTable(
				8, descHeap.GetGPUHandle(restirPrimaryHitBuffer->GetUAVHandle().GetIndex())
			);
			TransitionResourceIfNeeded(cmdList4.Get(), restirReservoirBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList4->SetComputeRootDescriptorTable(
				9, descHeap.GetGPUHandle(restirReservoirBuffer->GetUAVHandle().GetIndex())
			);
		}

		struct RestirCandidateConstants
		{
			uint32_t enabled;
			uint32_t screenWidth;
			uint32_t screenHeight;
			uint32_t pad0;
		};
		RestirCandidateConstants restirConstants = {restirCandidateEnabled ? 1u : 0u, m_width, m_height, 0u};
		cmdList4->SetComputeRoot32BitConstants(10, 4, &restirConstants, 0);
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
			DxUtils::CreateUAVBarrier(restirReservoirBuffer->GetResource())
		};
		cmdList4->ResourceBarrier(2, restirBarriers);
		TransitionResourceIfNeeded(
			cmdList4.Get(), restirPrimaryHitBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		TransitionResourceIfNeeded(
			cmdList4.Get(), restirReservoirBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		restirFinalReservoirData->reservoirBuffer = restirReservoirData->reservoirBuffer;
		restirFinalReservoirData->source = RestirFinalReservoirSource::Initial;
	}
}
