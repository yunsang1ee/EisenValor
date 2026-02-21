#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include <unordered_set>
#include <Scene.h>
#include <DxFrameResource.h>
#include <DxDeviceGlobal.h>
#include <DxRendererGlobal.h>
#include <DxDescriptorHeapGlobal.h>
#include <DxSamplerHeapGlobal.h>
#include <DxCommandQueueGlobal.h>
#include <InputGlobal.h>
#include <MeshComponent.h>
#include <DxSwapChain.h>
#include <DxBLAS.h>
#include <DxBuffer.h>
#include <DxUploadHeap.h>
#include <DxUtils.h>
#include "MeshResource.h"
#include "MaterialResource.h"
#include "TextureResource.h"

DxrRenderPass::DxrRenderPass(uint32_t width, uint32_t height) : m_width(width), m_height(height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Constructor: {}x{}\n", width, height);
}

void DxrRenderPass::Initialize()
{
	DEBUG_LOG_FMT("[DxrRenderPass] Initializing with resolution: {}x{}\n", m_width, m_height);

	auto&				  device = GLOBAL(DxDeviceGlobal);
	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));
	m_tlas = std::make_unique<DxTLAS>();
	m_tlas->Initialize(device5.Get(), 2000);

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	DEBUG_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	m_blasCache.clear();
	m_instanceBuffer.Clear();
	m_materialConstants.Clear();
	m_geoTable.Clear();

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

void DxrRenderPass::CollectRenderData(Scene* scene)
{
	auto* meshStorage = scene->GetStorage<MeshComponent>();
	if (nullptr == meshStorage)
	{
		return;
	}

	m_instanceBuffer.Clear();
	m_materialConstants.Clear();
	m_geoTable.Clear();

	std::unordered_map<EvAsset::Guid, uint32_t, EvAsset::GuidHash> materialToIndex;

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (false == meshComp.IsValid())
		{
			continue;
		}

		auto* meshRes = meshComp.GetMeshResource();

		uint32_t geoBaseIdx = static_cast<uint32_t>(m_geoTable.Size());

		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto*	 matRes = meshComp.GetMaterial(subMesh.materialSlot);
			uint32_t matIdx = 0;

			m_geoTable.Register(
				{.vertexBase = 0, // 단일 버퍼이므로 0
				 .indexBase = subMesh.indexOffset,
				 .materialIdx = matIdx,
				 .pad0 = 0}
			);

			if (nullptr == matRes)
			{
				DEBUG_LOG_FMT(
					"[DxrRenderPass] WARNING: Mesh '{}' has submesh with invalid material slot {}. Using default "
					"material.\n",
					meshRes->GetName(), subMesh.materialSlot
				);
				continue;
			}

			const auto& matGuid = matRes->GetGuid();
			if (true == materialToIndex.contains(matGuid))
			{
				matIdx = materialToIndex[matGuid];
				continue;
			}

			matIdx = static_cast<uint32_t>(m_materialConstants.Size());
			MaterialGPUData gpuMat = {};
			gpuMat.albedo = matRes->GetAlbedo();
			gpuMat.roughness = matRes->GetRoughness();
			gpuMat.metallic = matRes->GetMetallic();
			gpuMat.shadingModel = static_cast<uint32_t>(matRes->GetShadingModelId());
			gpuMat.materialFlags = matRes->GetMaterialFlags();

			if (auto albedoRes = matRes->GetTexture("ALBD"))
			{
				gpuMat.albedoTextureIdx = albedoRes->GetTexture()->GetSRVIndex();
			}

			if (auto normalRes = matRes->GetTexture("NRML"))
			{
				gpuMat.normalTextureIdx = normalRes->GetTexture()->GetSRVIndex();
			}

			if (auto ormRes = matRes->GetTexture("ORMS"))
			{
				gpuMat.ormTextureIdx = ormRes->GetTexture()->GetSRVIndex();
			}

			m_materialConstants.Register(gpuMat);
			materialToIndex[matGuid] = matIdx;
		}

		InstanceData   inst = {};
		DX::XMFLOAT4X4 worldFloat = meshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		DX::XMMATRIX   worldMat = DX::XMLoadFloat4x4(&worldFloat);
		DX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DX::XMStoreFloat4x4(&inst.worldIT, DX::XMMatrixTranspose(DX::XMMatrixInverse(nullptr, worldMat)));

		inst.vertexBufferIdx = meshRes->GetVertexBuffer()->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = meshComp.GetOwner().id;

		m_instanceBuffer.Register(inst);
	}
}

void DxrRenderPass::BuildAccelerationStructures(DxFrameResource* frame, Scene* scene)
{
	auto&				  device = GLOBAL(DxDeviceGlobal);
	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));

	auto*							   cmdList = frame->GetMainContext()->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	if (nullptr == scene)
		return;

	std::vector<std::pair<GameObject*, DxBLAS*>> instances;
	auto*										 meshStorage = scene->GetStorage<MeshComponent>();
	if (nullptr == meshStorage)
		return;

	for (auto& meshComp : meshStorage->GetList())
	{
		if (false == meshComp.IsValid())
		{
			continue;
		}

		auto*		meshRes = meshComp.GetMeshResource();
		const auto& guid = meshRes->GetGuid();

		auto it = m_blasCache.find(guid);
		if (m_blasCache.end() == it)
		{
			auto blas = std::make_unique<DxBLAS>();
			blas->Build(device5.Get(), cmdList4.Get(), meshRes, false, meshRes->GetName() + "_BLAS");

			auto barrier = DxUtils::CreateUAVBarrier(blas->GetResource());
			cmdList->ResourceBarrier(1, &barrier);

			m_blasCache[guid] = std::move(blas);
			it = m_blasCache.find(guid);

			DEBUG_LOG_FMT("[DxrRenderPass] Built BLAS for Asset: {}\n", meshRes->GetName());
		}

		auto* obj = meshComp.GetGameObject();
		if (nullptr != obj && it->second->IsBuilt())
		{
			instances.push_back({obj, it->second.get()});
		}
	}

	if (m_tlas && false == instances.empty())
	{
		if (m_needsRebuild || false == m_tlas->IsBuilt())
		{
			m_tlas->Build(device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), instances);
			m_needsRebuild = false;
		}
		else
		{
			m_tlas->Refit(device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), instances);
		}
	}
}

void DxrRenderPass::CreateRaytracingPipeline()
{
	auto&				  device = GLOBAL(DxDeviceGlobal);
	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));

	m_rtLitePipeline = std::make_unique<DxRtPipelineState>();
	m_rtLitePipeline->Create(device5.Get(), L"Resource/Shader/RaytracingPrimary.hlsl", 2);
	m_rtLiteShaderTable = std::make_unique<DxRtShaderTable>();
	m_rtLiteShaderTable->Build(device5.Get(), m_rtLitePipeline.get(), 1);

	m_rtPipeline = std::make_unique<DxRtPipelineState>();
	m_rtPipeline->Create(device5.Get(), L"Resource/Shader/RaytracingLibrary.hlsl", 4);
	m_rtShaderTable = std::make_unique<DxRtShaderTable>();
	m_rtShaderTable->Build(device5.Get(), m_rtPipeline.get(), 1);

	m_ptPipeline = std::make_unique<DxRtPipelineState>();
	m_ptPipeline->Create(device5.Get(), L"Resource/Shader/RaytracingLibraryPT.hlsl", 19);
	m_ptShaderTable = std::make_unique<DxRtShaderTable>();
	m_ptShaderTable->Build(device5.Get(), m_ptPipeline.get(), 1);

	DEBUG_LOG_FMT("[GameFramework] Raytracing pipeline created\n");
}

void DxrRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!m_initialized || !scene)
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

	auto& device = GLOBAL(DxDeviceGlobal);
	auto* context = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();

	CollectRenderData(scene);
	BuildAccelerationStructures(frame, scene);

	m_instanceBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_materialConstants.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_geoTable.SyncToGPU(device.GetDevice(), *context, *uploadHeap);

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

	auto mainCam = CameraComponent::GetMainViewMatrix();
	auto mainProj = CameraComponent::GetMainProjectionMatrix();
	auto viewProjInv = DX::XMMatrixTranspose(DX::XMMatrixInverse(nullptr, DX::XMMatrixMultiply(mainCam, mainProj)));
	DX::XMFLOAT4X4 viewProjInvFloat;
	DX::XMStoreFloat4x4(&viewProjInvFloat, viewProjInv);
	cmdList4->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);

	// 바인딩: t1(Instance), t2(Material), t3(GeoTable)
	if (m_instanceBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(3, m_instanceBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress());
	}
	if (m_materialConstants.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(4, m_materialConstants.GetBuffer()->GetResource()->GetGPUVirtualAddress());
	}
	if (m_geoTable.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(5, m_geoTable.GetBuffer()->GetResource()->GetGPUVirtualAddress());
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
