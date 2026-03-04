#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include "Scene.h"
#include "DxFrameResource.h"
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

#include <unordered_map>

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

	m_tlas = std::make_unique<DxTLAS>();
	m_tlas->Initialize(m_device5.Get(), 2'000);

	m_instanceData = std::make_shared<InstanceRenderData>();
	m_materialData = std::make_shared<MaterialRenderData>();
	m_geoTableData = std::make_shared<GeoTableRenderData>();

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	DEBUG_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	m_instanceData.reset();
	m_materialData.reset();
	m_geoTableData.reset();
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
}

void DxrRenderPass::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	if (m_raytracingOutput.HasUAV(0))
	{
		auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
		auto  fenceValue = commandQueue.GetCurrentFenceValue();
		m_raytracingOutput.ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
	}

	m_raytracingOutput.Initialize(
		device.GetDevice(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "DxrRenderPass_Output"
	);
	m_raytracingOutput.CreateUAV(device.GetDevice(), descHeap, 0);

	DEBUG_LOG_FMT(
		"[DxrRenderPass] Raytracing output created: {}x{}, UAV Index={}\n", width, height,
		m_raytracingOutput.GetUAVIndex(0)
	);
}

void DxrRenderPass::PrepareRenderData(DxFrameResource* frame, Scene* scene)
{
	m_instanceData->syncBuffer.Clear();
	m_materialData->syncBuffer.Clear();
	m_geoTableData->syncBuffer.Clear();
	m_materialData->materialToIndex.clear();

	std::vector<std::pair<GameObject*, DxBLAS*>> tlasInstances;
	tlasInstances.reserve(1'000);

	auto*							   cmdList = frame->GetMainContext()->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	CollectStaticMeshData(scene, cmdList4.Get(), tlasInstances);
	CollectSkinnedMeshData(scene, cmdList4.Get(), tlasInstances);

	if (nullptr != m_tlas && false == tlasInstances.empty())
	{
		bool shouldRebuild =
			(false == m_tlas->IsBuilt()) || (m_tlas->GetInstanceCount() != (uint32_t)tlasInstances.size());
		if (shouldRebuild)
		{
			m_tlas->Build(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), tlasInstances);
		}
		else
		{
			m_tlas->Refit(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), tlasInstances);
		}

		m_instanceData->tlasDescriptorIndex = m_tlas->GetSRVIndex();
		m_instanceData->tlasAddress = m_tlas->GetGPUAddress();
	}
}

void DxrRenderPass::CollectStaticMeshData(
	Scene* scene, ID3D12GraphicsCommandList4* cmdList, std::vector<std::pair<GameObject*, DxBLAS*>>& tlasInstances
)
{
	auto* meshStorage = scene->GetStorage<MeshComponent>();
	if (nullptr == meshStorage)
	{
		return;
	}

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (false == meshComp.IsValid())
		{
			DEBUG_LOG_FMT("[DxrRenderPass] WARNING: Found invalid MeshComponent in storage. Skipping this component.\n"
			);
			continue;
		}

		auto* meshRes = meshComp.GetMeshResource();
		if (nullptr == meshRes || nullptr == meshRes->GetVertexBuffer() || nullptr == meshRes->GetIndexBuffer())
		{
			DEBUG_LOG_FMT("[DxrRenderPass] WARNING: MeshComponent's MeshResource is invalid or missing buffers. "
						  "Skipping this component.\n");
			continue;
		}

		auto* blas = meshRes->GetBLAS();
		if (nullptr == blas || false == blas->IsBuilt())
		{
			continue;
		}

		uint32_t geoBaseIdx = static_cast<uint32_t>(m_geoTableData->syncBuffer.Size());
		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto* matRes = meshComp.GetMaterial(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				DEBUG_LOG_FMT(
					"[DxrRenderPass] WARNING: Mesh '{}' has submesh with invalid material slot {}. Skipping this "
					"submesh.\n",
					meshRes->GetName(), subMesh.materialSlot
				);
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}

			uint32_t	matIdx = 0;
			const auto& matGuid = matRes->GetGuid();

			if (m_materialData->materialToIndex.contains(matGuid))
			{
				matIdx = m_materialData->materialToIndex[matGuid];
			}
			else
			{
				matIdx = static_cast<uint32_t>(m_materialData->syncBuffer.Size());
				MaterialGPUData gpuMat = {};
				gpuMat.albedo = matRes->GetAlbedo();
				gpuMat.roughness = matRes->GetRoughness();
				gpuMat.metallic = matRes->GetMetallic();
				gpuMat.shadingModel = static_cast<uint32_t>(matRes->GetShadingModelId());
				gpuMat.materialFlags = matRes->GetMaterialFlags();
				gpuMat.albedoTextureIdx =
					matRes->GetTexture("ALBD") ? matRes->GetTexture("ALBD")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.normalTextureIdx =
					matRes->GetTexture("NRML") ? matRes->GetTexture("NRML")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.ormTextureIdx =
					matRes->GetTexture("ORMS") ? matRes->GetTexture("ORMS")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.emissiveTextureIdx =
					matRes->GetTexture("EMSV") ? matRes->GetTexture("EMSV")->GetTexture()->GetSRVIndex() : ~0u;

				m_materialData->syncBuffer.Register(gpuMat);
				m_materialData->materialToIndex[matGuid] = matIdx;
			}

			m_geoTableData->syncBuffer.Register(
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

		m_instanceData->syncBuffer.Register(inst);
		tlasInstances.push_back({meshComp.GetGameObject(), blas});
	}
}

void DxrRenderPass::CollectSkinnedMeshData(
	Scene* scene, ID3D12GraphicsCommandList4* cmdList, std::vector<std::pair<GameObject*, DxBLAS*>>& tlasInstances
)
{
	auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
	if (nullptr == skinnedMeshStorage)
	{
		return;
	}

	for (const auto& skinnedMeshComp : skinnedMeshStorage->GetList())
	{
		if (false == skinnedMeshComp.IsValid())
		{
			continue;
		}

		auto* meshRes = skinnedMeshComp.GetSkinnedMeshResource();
		auto* skinnedVB = skinnedMeshComp.GetSkinnedVertexBuffer();
		if (nullptr == meshRes || nullptr == skinnedVB)
		{
			continue;
		}

		if (nullptr == skinnedMeshComp.GetBLAS())
		{
			auto newBlas = std::make_unique<DxBLAS>();
			newBlas->Build(
				m_device5.Get(), cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(),
				sizeof(EvAsset::Vertex), meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetSubMeshes(), true,
				meshRes->GetName() + "_AnimatedBLAS"
			);

			const_cast<SkinnedMeshComponent&>(skinnedMeshComp).SetBLAS(std::move(newBlas));
		}
		else
		{
			skinnedMeshComp.GetBLAS()->Refit(
				cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(), sizeof(EvAsset::Vertex),
				meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetSubMeshes()
			);
		}

		uint32_t geoBaseIdx = static_cast<uint32_t>(m_geoTableData->syncBuffer.Size());
		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto* matRes = skinnedMeshComp.GetMaterialResource(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				DEBUG_LOG_FMT(
					"[DxrRenderPass] WARNING: Mesh '{}' has submesh with invalid material slot {}. Skipping this "
					"submesh.\n",
					meshRes->GetName(), subMesh.materialSlot
				);
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}

			uint32_t	matIdx = 0;
			const auto& matGuid = matRes->GetGuid();

			if (m_materialData->materialToIndex.contains(matGuid))
			{
				matIdx = m_materialData->materialToIndex[matGuid];
			}
			else
			{
				matIdx = static_cast<uint32_t>(m_materialData->syncBuffer.Size());
				MaterialGPUData gpuMat = {};
				gpuMat.albedo = matRes->GetAlbedo();
				gpuMat.roughness = matRes->GetRoughness();
				gpuMat.metallic = matRes->GetMetallic();
				gpuMat.shadingModel = static_cast<uint32_t>(matRes->GetShadingModelId());
				gpuMat.materialFlags = matRes->GetMaterialFlags();
				gpuMat.albedoTextureIdx =
					matRes->GetTexture("ALBD") ? matRes->GetTexture("ALBD")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.normalTextureIdx =
					matRes->GetTexture("NRML") ? matRes->GetTexture("NRML")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.ormTextureIdx =
					matRes->GetTexture("ORMS") ? matRes->GetTexture("ORMS")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.emissiveTextureIdx =
					matRes->GetTexture("EMSV") ? matRes->GetTexture("EMSV")->GetTexture()->GetSRVIndex() : ~0u;

				m_materialData->syncBuffer.Register(gpuMat);
				m_materialData->materialToIndex[matGuid] = matIdx;
			}

			m_geoTableData->syncBuffer.Register(
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

		m_instanceData->syncBuffer.Register(inst);
		tlasInstances.push_back({skinnedMeshComp.GetGameObject(), skinnedMeshComp.GetBLAS()});
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
	if (!m_initialized || nullptr == scene || nullptr == renderContext)
	{
		return;
	}

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
	}
	if (input.GetInputDown(VK_F7))
	{
		m_useLiteRT = !m_useLiteRT;
	}

	renderContext->SetData(m_instanceData, 0);
	renderContext->SetData(m_materialData, 0);
	renderContext->SetData(m_geoTableData, 0);

	auto& device = GLOBAL(DxDeviceGlobal);
	auto* context = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();

	PrepareRenderData(frame, scene);

	m_instanceData->syncBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_materialData->syncBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_geoTableData->syncBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);

	if (nullptr == m_tlas || false == m_tlas->IsBuilt())
	{
		return;
	}

	auto*							   cmdList = context->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);
	auto& samplerHeap = GLOBAL(DxSamplerHeapGlobal);

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

	cmdList4->SetComputeRootDescriptorTable(0, descHeap.GetGPUHandle(m_tlas->GetSRVIndex()));
	cmdList4->SetComputeRootDescriptorTable(1, descHeap.GetGPUHandle(m_raytracingOutput.GetUAVIndex(0)));

	auto cameraData = renderContext->GetData<CameraRenderData>();
	if (nullptr != cameraData)
	{
		DX::XMFLOAT4X4 viewProjInvFloat;
		DirectX::XMStoreFloat4x4(&viewProjInvFloat, DirectX::XMMatrixTranspose(cameraData->viewProjInverse));
		cmdList4->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);
	}

	if (m_instanceData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			3, m_instanceData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (m_materialData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			4, m_materialData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (m_geoTableData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			5, m_geoTableData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}

	auto* shaderTable =
		m_usePathTracing ? m_ptShaderTable.get() : (m_useLiteRT ? m_rtLiteShaderTable.get() : m_rtShaderTable.get());
	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = shaderTable->GetRayGenRecord(),
		.MissShaderTable = shaderTable->GetMissTable(),
		.HitGroupTable = shaderTable->GetHitGroupTable(),
		.Width = m_raytracingOutput.GetWidth(),
		.Height = m_raytracingOutput.GetHeight(),
		.Depth = 1
	};

	cmdList4->DispatchRays(&desc);
}
