#include "stdafxClient.h"
#include "SkinningPass.h"
#include <Scene.h>
#include <DxFrameResource.h>
#include <DxDeviceGlobal.h>
#include <DxRendererGlobal.h>
#include <DxShaderCompilerGlobal.h>
#include <DxDescriptorHeapGlobal.h>
#include <DxUtils.h>
#include "SkinnedMeshComponent.h"
#include "SkinnedMeshResource.h"
#include "DxBuffer.h"
#include "DxCommandContext.h"

void SkinningPass::Initialize()
{
	CreatePipeline();
	m_initialized = true;
	DEBUG_LOG_FMT("[SkinningPass] Initialized\n");
}

void SkinningPass::Release()
{
	m_boneMatrixBuffer.Clear();
	m_pso.Reset();
	m_rootSignature.Reset();
	m_initialized = false;
	DEBUG_LOG_FMT("[SkinningPass] Released\n");
}

void SkinningPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* context)
{
	if (!m_initialized || nullptr == scene)
	{
		return;
	}

	auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
	if (nullptr == skinnedMeshStorage || skinnedMeshStorage->GetList().empty())
	{
		return;
	}

	auto& deviceG = GLOBAL(DxDeviceGlobal);
	auto* device = deviceG.GetDevice();
	auto* cmdContext = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();
	auto* cmdList = cmdContext->CommandList();

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	m_boneMatrixBuffer.Clear();
	for (auto& skinnedMeshComp : skinnedMeshStorage->GetList())
	{
		if (false == skinnedMeshComp.IsValid())
		{
			continue;
		}

		const auto& finalMatrices = skinnedMeshComp.GetFinalMatrices();
		for (const auto& mat : finalMatrices)
		{
			m_boneMatrixBuffer.Register(mat);
		}
	}

	if (m_boneMatrixBuffer.Size() == 0)
	{
		return;
	}

	m_boneMatrixBuffer.SyncToGPU(device, *cmdContext, *uploadHeap);

	cmdList->SetComputeRootSignature(m_rootSignature.Get());

	cmdList4->SetPipelineState(m_pso.Get());

	uint32_t currentBoneBase = 0;

	for (auto& skinnedMeshComp : skinnedMeshStorage->GetList())
	{
		if (false == skinnedMeshComp.IsValid())
		{
			continue;
		}

		auto*	 meshRes = skinnedMeshComp.GetSkinnedMeshResource();
		uint32_t vertexCount = meshRes->GetVertexCount();

		auto* outVB = skinnedMeshComp.GetSkinnedVertexBuffer();
		if (nullptr == outVB)
		{
			auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);
			auto  buffer = std::make_unique<DxBuffer>();
			buffer->Initialize(
				device, sizeof(EvAsset::Vertex) * vertexCount, EBufferUsage::RawBuffer,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
				meshRes->GetName() + "_SkinnedVB"
			);

			buffer->CreateSRV(device, descHeap, vertexCount, sizeof(EvAsset::Vertex));
			skinnedMeshComp.SetSkinnedVertexBuffer(std::move(buffer));
			outVB = skinnedMeshComp.GetSkinnedVertexBuffer();
		}

		auto* inVB = meshRes->GetVertexBuffer();

		D3D12_RESOURCE_BARRIER preBarrier = DxUtils::CreateTransitionBarrier(
			outVB->GetResource(), outVB->GetCurrentState(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		cmdList->ResourceBarrier(1, &preBarrier);
		outVB->SetState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		uint32_t constants[4] = {vertexCount, currentBoneBase, 0, 0};
		cmdList->SetComputeRoot32BitConstants(0, 4, constants, 0);
		cmdList->SetComputeRootShaderResourceView(1, inVB->GetResource()->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(
			2, m_boneMatrixBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
		cmdList->SetComputeRootUnorderedAccessView(3, outVB->GetResource()->GetGPUVirtualAddress());

		cmdList->Dispatch((vertexCount + 255) / 256, 1, 1);

		D3D12_RESOURCE_BARRIER postBarriers[2];
		postBarriers[0] = DxUtils::CreateUAVBarrier(outVB->GetResource());
		postBarriers[1] = DxUtils::CreateTransitionBarrier(
			outVB->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		cmdList->ResourceBarrier(2, postBarriers);
		outVB->SetState(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		currentBoneBase += (uint32_t)skinnedMeshComp.GetFinalMatrices().size();
	}
}

void SkinningPass::CreatePipeline()
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
	ThrowIfFailed(device.GetDevice()->CreateRootSignature(
		0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)
	));


	// 2. Compute Shader 컴파일
	auto& compiler = GLOBAL(DxShaderCompilerGlobal);
	auto  csBlob =
		compiler.CompileShaderFromFile(L"SkinningCompute", L"Resource/Shader/SkinningCompute.hlsl", "main", "cs_6_6");
	if (!csBlob)
	{
		DEBUG_LOG_FMT("[DxrRenderPass] ERROR: Failed to compile SkinningCompute.hlsl\n");
		return;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.CS = {csBlob->GetBufferPointer(), csBlob->GetBufferSize()};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	ThrowIfFailed(device.GetDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

	DEBUG_LOG_FMT("[DxrRenderPass] Skinning compute pipeline created successfully\n");
}
