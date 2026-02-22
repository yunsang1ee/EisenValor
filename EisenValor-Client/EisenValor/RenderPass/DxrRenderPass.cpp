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
#include <DxShaderCompilerGlobal.h>
#include <InputGlobal.h>
#include <MeshComponent.h>
#include "SkinnedMeshComponent.h"
#include <DxSwapChain.h>
#include <DxBLAS.h>
#include <DxBuffer.h>
#include <DxUploadHeap.h>
#include <DxUtils.h>
#include "MeshResource.h"
#include "SkinnedMeshResource.h"
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

	CreateSkinningPipeline(); 
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
	m_boneMatrixBuffer.Clear();	//Bone 데이터 수집

	std::unordered_map<EvAsset::Guid, uint32_t, EvAsset::GuidHash> materialToIndex;

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (false == meshComp.IsValid())
		{
			continue;
		}

		auto*	 meshRes = meshComp.GetMeshResource();
		uint32_t geoBaseIdx = static_cast<uint32_t>(m_geoTable.Size());

		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto* matRes = meshComp.GetMaterial(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				DEBUG_LOG_FMT(
					"[DxrRenderPass] WARNING: Mesh has submesh with invalid material slot. Skipping this submesh.\n"
				);
				continue;
			}

			uint32_t	matIdx = 0;
			const auto& matGuid = matRes->GetGuid();

			if (materialToIndex.contains(matGuid))
			{
				matIdx = materialToIndex[matGuid];
			}
			else
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

			m_geoTable.Register(
				{.vertexBase = 0, // 단일 버퍼이므로 0
				 .indexBase = subMesh.indexOffset,
				 .materialIdx = matIdx,
				 .pad0 = 0}
			);
		}

		const uint32_t vbIdx = meshRes->GetVertexBuffer()->GetSRVIndex();
		const uint32_t ibIdx = meshRes->GetIndexBuffer()->GetSRVIndex();

		if (vbIdx == ~0u || ibIdx == ~0u)
		{
			DEBUG_LOG_FMT(
				"[DxrRenderPass] WARNING: Mesh '{}' has invalid VB/IB SRV index (VB={}, IB={}). Skipping instance.\n",
				meshRes->GetName(), vbIdx, ibIdx
			);
			continue;
		}

		InstanceData   inst = {};
		DX::XMFLOAT4X4 worldFloat = meshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		DX::XMMATRIX   worldMat = DX::XMLoadFloat4x4(&worldFloat);
		DX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DX::XMStoreFloat4x4(&inst.worldIT, DX::XMMatrixTranspose(DX::XMMatrixInverse(nullptr, worldMat)));

		inst.vertexBufferIdx = vbIdx;
		inst.indexBufferIdx = ibIdx;
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = meshComp.GetOwner().id;

		DEBUG_LOG_FMT("[DxrRenderPass] STATIC '{}' Debug: GeoBase={}, Flags={}\n", meshRes->GetName(), inst.geoInfoBaseIdx, inst.instanceFlags);

		m_instanceBuffer.Register(inst);
	}

    // SkinnedMeshComponent 처리
	// Bone 행렬을 버퍼에 등록하고 InstanceData 처리
    auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
    if (nullptr != skinnedMeshStorage)
    {
		for (SkinnedMeshComponent& skinnedMeshComp : skinnedMeshStorage->GetList())
        {
            if (false == skinnedMeshComp.IsValid())
            {
                continue;
            }

            auto* skinnedMeshRes = skinnedMeshComp.GetSkinnedMeshResource();
            uint32_t geoBaseIdx = static_cast<uint32_t>(m_geoTable.Size());

            // Bone 행렬 등록
            const auto& finalMatrices = skinnedMeshComp.GetFinalMatrices();
            uint32_t boneMatrixBaseIdx = static_cast<uint32_t>(m_boneMatrixBuffer.Size());
 /*           DEBUG_LOG_FMT(
                "[DxrRenderPass] SkinnedMesh Bone Matrix Count for {}({}): {} (BoneBaseIdx was {})\n",
                skinnedMeshComp.GetGameObject()->GetName(), skinnedMeshRes->GetName(),
                finalMatrices.size(), boneMatrixBaseIdx
            );*/
            if (!finalMatrices.empty())
            {
                for (const auto& mat : finalMatrices)
                {
                    m_boneMatrixBuffer.Register(mat);
                }
            }

            for (const auto& subMesh : skinnedMeshRes->GetSubMeshes())
            {
                auto* matRes = skinnedMeshComp.GetMaterialResource(subMesh.materialSlot);
                if (nullptr == matRes)
                {
                    DEBUG_LOG_FMT(
                        "[DxrRenderPass] WARNING: SkinnedMesh '{}' has submesh with invalid material slot {}. Skipping this submesh.\n",
                        skinnedMeshRes->GetName(), subMesh.materialSlot
                    );
                    continue;
                }

                uint32_t matIdx = 0;
                const auto& matGuid = matRes->GetGuid();

                if (materialToIndex.contains(matGuid))
                {
                    matIdx = materialToIndex[matGuid];
                }
                else
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

                m_geoTable.Register(
                    {.vertexBase = 0, // 단일 버퍼이므로 0
                     .indexBase = subMesh.indexOffset,
                     .materialIdx = matIdx,
                     .pad0 = 0}
                );
            }

            const uint32_t vbIdx = skinnedMeshRes->GetVertexBuffer()->GetSRVIndex();
            const uint32_t ibIdx = skinnedMeshRes->GetIndexBuffer()->GetSRVIndex();

            if (vbIdx == ~0u || ibIdx == ~0u)
            {
                DEBUG_LOG_FMT(
                    "[DxrRenderPass] WARNING: SkinnedMesh '{}' has invalid VB/IB SRV index (VB={}, IB={}). Skipping instance.\n",
                    skinnedMeshRes->GetName(), vbIdx, ibIdx
                );
                continue;
            }

            InstanceData inst = {};
            DX::XMFLOAT4X4 worldFloat = skinnedMeshComp.GetGameObject()->GetTransform().GetWorldMatrix();
            DX::XMMATRIX worldMat = DX::XMLoadFloat4x4(&worldFloat);
            DX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
            DX::XMStoreFloat4x4(&inst.worldIT, DX::XMMatrixTranspose(DX::XMMatrixInverse(nullptr, worldMat)));

            // 1. 결과 버퍼를 미리 생성 (SRV 인덱스 확보)
            if (nullptr == skinnedMeshComp.GetSkinnedVertexBuffer())
            {
                auto& device = GLOBAL(DxDeviceGlobal);
                auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);
                uint32_t vCount = skinnedMeshRes->GetVertexCount();
                auto buffer = std::make_unique<DxBuffer>();
                buffer->Initialize(
                    device.GetDevice(), 
                    sizeof(EvAsset::Vertex) * vCount, 
                    EBufferUsage::RawBuffer, 
                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_COMMON,
                    skinnedMeshRes->GetName() + "_PostSkinningVB"
                );
                buffer->CreateSRV(device.GetDevice(), descHeap, vCount, sizeof(EvAsset::Vertex));
                skinnedMeshComp.SetSkinnedVertexBuffer(std::move(buffer));
            }

            inst.vertexBufferIdx = skinnedMeshComp.GetSkinnedVertexBuffer()->GetSRVIndex();
            inst.indexBufferIdx = ibIdx;
            inst.geoInfoBaseIdx = geoBaseIdx;
            inst.instanceID = skinnedMeshComp.GetOwner().id;

            inst.instanceFlags = 1; 
            inst.boneMatrixBaseIdx = boneMatrixBaseIdx; 
            m_instanceBuffer.Register(inst);
        }
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

    // SkinnedMeshComponent BLAS Build
    auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
    if (nullptr != skinnedMeshStorage)
    {
		for (SkinnedMeshComponent& skinnedMeshComp : skinnedMeshStorage->GetList())
        {
            if (false == skinnedMeshComp.IsValid())
            {
                continue;
            }

            auto* skinnedMeshRes = skinnedMeshComp.GetSkinnedMeshResource();
            auto* skinnedVB = skinnedMeshComp.GetSkinnedVertexBuffer();
            if (!skinnedVB) continue;
            // 캐릭터 전용 BLAS 사용
            if (nullptr == skinnedMeshComp.GetBLAS())
            {
                auto newBlas = std::make_unique<DxBLAS>();
                newBlas->Build(device5.Get(), cmdList4.Get(), skinnedMeshRes, true, "AnimatedBLAS", skinnedVB->GetResource()->GetGPUVirtualAddress());
                skinnedMeshComp.SetBLAS(std::move(newBlas));
            }
            else
            {
                skinnedMeshComp.GetBLAS()->Build(device5.Get(), cmdList4.Get(), skinnedMeshRes, true, "AnimatedBLAS", skinnedVB->GetResource()->GetGPUVirtualAddress());
            }

            auto* obj = skinnedMeshComp.GetGameObject();
            if (nullptr != obj && skinnedMeshComp.GetBLAS()->IsBuilt())
            {
                instances.push_back({obj, skinnedMeshComp.GetBLAS()});
            }
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

		auto barrier = DxUtils::CreateUAVBarrier(m_tlas->GetResource());
		cmdList->ResourceBarrier(1, &barrier);
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
	
	m_instanceBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_materialConstants.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_geoTable.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_boneMatrixBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);

	// GPU Skinning -> BLAS Build
	UpdateSkinning(frame, scene);

	BuildAccelerationStructures(frame, scene);

	if (nullptr == m_tlas || false == m_tlas->IsBuilt())
	{
		return;
	}

	if (!m_instanceBuffer.GetBuffer() || !m_materialConstants.GetBuffer() || !m_geoTable.GetBuffer())
	{
		DEBUG_LOG_FMT(
			"[DxrRenderPass] Skipping DispatchRays: required buffers not ready "
			"(inst={}, mat={}, geo={})\n",
			m_instanceBuffer.GetBuffer() != nullptr, m_materialConstants.GetBuffer() != nullptr,
			m_geoTable.GetBuffer() != nullptr
		);
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

	// 바인딩: t1(Instance), t2(Material), t3(GeoTable), t4(BoneMatrix)
	if (m_instanceBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			3, m_instanceBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (m_materialConstants.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			4, m_materialConstants.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
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

void DxrRenderPass::CreateSkinningPipeline()
{
	auto& device = GLOBAL(DxDeviceGlobal);

	// 1. Root Signature 정의
	// Param 0: Root Constants (b0) - VertexCount, BoneBaseIndex
	// Param 1: SRV (t0) - SkinnedVertex
	// Param 2: SRV (t1) - BoneMatrices
	// Param 3: UAV (u0) - Vertex
	D3D12_ROOT_PARAMETER rootParams[4] = {};

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[0].Constants.ShaderRegister = 0;
	rootParams[0].Constants.RegisterSpace = 0;
	rootParams[0].Constants.Num32BitValues = 4;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[1].Descriptor.ShaderRegister = 0;
	rootParams[1].Descriptor.RegisterSpace = 0;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[2].Descriptor.ShaderRegister = 1;
	rootParams[2].Descriptor.RegisterSpace = 0;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParams[3].Descriptor.ShaderRegister = 0;
	rootParams[3].Descriptor.RegisterSpace = 0;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = _countof(rootParams);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> errorBlob;
	if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob)))
	{
		if (errorBlob)
		{
			DEBUG_LOG_FMT("[DxrRenderPass] RootSig Error: {}\n", (const char*)errorBlob->GetBufferPointer());
		}
		return;
	}
	ThrowIfFailed(device.GetDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_skinningRootSignature)));

	// 2. Compute Shader 컴파일
	auto& compiler = GLOBAL(DxShaderCompilerGlobal);
	auto  csBlob = compiler.CompileShaderFromFile(
		 L"SkinningCompute", L"Resource/Shader/SkinningCompute.hlsl", "main", "cs_6_6"
	 );
	if (!csBlob)
	{
		DEBUG_LOG_FMT("[DxrRenderPass] ERROR: Failed to compile SkinningCompute.hlsl\n");
		return;
	}

	// 3. PSO 생성
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_skinningRootSignature.Get();
	psoDesc.CS = {csBlob->GetBufferPointer(), csBlob->GetBufferSize()};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	ThrowIfFailed(device.GetDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_skinningPSO)));

	DEBUG_LOG_FMT("[DxrRenderPass] Skinning compute pipeline created successfully\n");
}

void DxrRenderPass::UpdateSkinning(DxFrameResource* frame, Scene* scene)
{
	auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
	if (nullptr == skinnedMeshStorage || skinnedMeshStorage->GetList().empty())
		return;

	auto* cmdList = frame->GetMainContext()->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	cmdList4->SetComputeRootSignature(m_skinningRootSignature.Get());
	cmdList4->SetPipelineState(m_skinningPSO.Get());

	uint32_t currentBoneBase = 0;

	for (SkinnedMeshComponent& skinnedMeshComp : skinnedMeshStorage->GetList())
	{
		if (false == skinnedMeshComp.IsValid()) continue;

		auto* meshRes = skinnedMeshComp.GetSkinnedMeshResource();
		uint32_t vertexCount = meshRes->GetVertexCount();

        // 버퍼는 CollectRenderData에서 생성됨
		auto* outVB = skinnedMeshComp.GetSkinnedVertexBuffer();
		auto* inVB = meshRes->GetVertexBuffer();

		// 2. 상태 전이 (UAV)
		D3D12_RESOURCE_BARRIER preBarrier = DxUtils::CreateTransitionBarrier(outVB->GetResource(), outVB->GetCurrentState(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cmdList4->ResourceBarrier(1, &preBarrier);
		outVB->SetState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// 3. 파라미터 연결
		// Root Constant: [VertexCount, BoneBaseIndex, Pad, Pad]
		uint32_t constants[4] = { vertexCount, currentBoneBase, 0, 0 };
		cmdList4->SetComputeRoot32BitConstants(0, 4, constants, 0);
		// t0: 원본 버퍼 (80바이트)
		cmdList4->SetComputeRootShaderResourceView(1, inVB->GetResource()->GetGPUVirtualAddress());
		// t1: 전체 bone matrix 버퍼
		cmdList4->SetComputeRootShaderResourceView(2, m_boneMatrixBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress());
		// u0: 결과 버퍼 (48바이트)
		cmdList4->SetComputeRootUnorderedAccessView(3, outVB->GetResource()->GetGPUVirtualAddress());

		// 4. 실행
		cmdList4->Dispatch((vertexCount + 255) / 256, 1, 1);

		// 5. 완료 대기 및 상태 전이 (UAV -> Read)
		D3D12_RESOURCE_BARRIER barriers[2];
		barriers[0] = DxUtils::CreateUAVBarrier(outVB->GetResource());
		barriers[1] = DxUtils::CreateTransitionBarrier(outVB->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cmdList4->ResourceBarrier(2, barriers);
		outVB->SetState(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		currentBoneBase += (uint32_t)skinnedMeshComp.GetFinalMatrices().size();
	}
}
