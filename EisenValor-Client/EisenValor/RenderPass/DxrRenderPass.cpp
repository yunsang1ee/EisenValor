#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include <unordered_set>
#include <Scene.h>
#include <DxFrameResource.h>
#include <DxDeviceGlobal.h>
#include <DxRendererGlobal.h>
#include <DxDescriptorHeapGlobal.h>
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

	std::unordered_map<EvAsset::Guid, uint32_t, EvAsset::GuidHash> materialToIndex;

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (!meshComp.IsValid())
		{
			continue;
		}

		auto* meshRes = meshComp.GetMeshResource();
		auto* matRes = meshComp.GetMaterial(0);

		uint32_t matIdx = 0;
		if (nullptr != matRes)
		{
			const auto& matGuid = matRes->GetGuid();
			if (!materialToIndex.contains(matGuid))
			{
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
			else
			{
				matIdx = materialToIndex[matGuid];
			}
		}

		// 인스턴스 정보 등록
		InstanceData   inst = {};
		DX::XMFLOAT4X4 worldFloat = meshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		DX::XMMATRIX   worldMat = DX::XMLoadFloat4x4(&worldFloat);
		DX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DX::XMStoreFloat4x4(&inst.worldIT, DX::XMMatrixTranspose(DX::XMMatrixInverse(nullptr, worldMat)));

		inst.vertexBufferIdx = meshRes->GetVertexBuffer()->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.materialIdx = matIdx;
		inst.instanceID = meshComp.GetOwner().id;

		m_instanceBuffer.Register(inst);
	}
}

void DxrRenderPass::BuildAccelerationStructures(DxFrameResource* frame, Scene* scene)
{
	auto& renderer = GLOBAL(DxRendererGlobal);
	auto& device = GLOBAL(DxDeviceGlobal);

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
			continue;

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

		// 인스턴스 목록에 추가
		auto* obj = meshComp.GetGameObject();
		if (nullptr != obj && it->second->IsBuilt())
		{
			instances.push_back({obj, it->second.get()});
		}
	}

	// TLAS Update
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
		return;

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
		DEBUG_LOG_FMT("[GameFramework] Switched to {}\n", m_usePathTracing ? "Path Tracing" : "Ray Tracing");
	}
	if (input.GetInputDown(VK_F7))
	{
		m_useLiteRT = !m_useLiteRT;
		DEBUG_LOG_FMT("[GameFramework] Switched to {}\n", m_useLiteRT ? "Lite Ray Tracing" : "Ray Tracing");
	}

	auto& device = GLOBAL(DxDeviceGlobal);
	auto* context = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();

	// 1. 데이터 수집 및 BLAS 확인
	CollectRenderData(scene);
	BuildAccelerationStructures(frame, scene);

	// 2. 인스턴스/머티리얼 버퍼 GPU 동기화
	m_instanceBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_materialConstants.SyncToGPU(device.GetDevice(), *context, *uploadHeap);

	// 3. DXR Dispatch
	if (nullptr == m_tlas || false == m_tlas->IsBuilt())
		return;

	auto*							   cmdList = context->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

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

	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList4->SetDescriptorHeaps(1, heaps);

	// Root Parameters (기존 인덱스와 호환성 확인 필요)
	// 0: TLAS
	D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV = descHeap.GetGPUHandle(m_tlas->GetSRVIndex());
	cmdList4->SetComputeRootDescriptorTable(0, tlasSRV);

	// 1: Output UAV
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV = descHeap.GetGPUHandle(m_raytracingOutput.GetUAVIndex(0));
	cmdList4->SetComputeRootDescriptorTable(1, outputUAV);

	// 2: Camera Constants
	auto mainCam = CameraComponent::GetMainViewMatrix();
	auto mainProj = CameraComponent::GetMainProjectionMatrix();
	auto viewProj = DX::XMMatrixMultiply(mainCam, mainProj);
	auto viewProjInv = DX::XMMatrixInverse(nullptr, viewProj);
	viewProjInv = DX::XMMatrixTranspose(viewProjInv);

	DX::XMFLOAT4X4 viewProjInvFloat;
	DX::XMStoreFloat4x4(&viewProjInvFloat, viewProjInv);

	cmdList4->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);

	// [FIX] 3: Geometry Descriptor Table -> 이제 Instance/Material 버퍼를 넘겨야 함
	// 기존 코드는 Range를 썼지만, Bindless에서는 InstanceBuffer SRV 하나만 넘기면 됨
	// 단, 셰이더가 아직 구형(Range) 방식을 쓰고 있다면 이 부분에서 셰이더 수정이 필요함.
	// 우선 m_instanceBuffer의 SRV를 넘기는 것으로 가정.
	if (m_instanceBuffer.GetBuffer())
	{
		auto handle = descHeap.GetGPUHandle(m_instanceBuffer.GetSRVIndex());
		cmdList4->SetComputeRootDescriptorTable(3, handle);
	}
	// 4: Material Buffer SRV (추가 필요 가능성 있음)

	// Dispatch
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
