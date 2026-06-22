#include "stdafxClient.h"
#include "RestirFinalEvaluationPass.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include "RenderData/RestirCandidateRenderData.h"
#include "RenderData/RestirFinalReservoirRenderData.h"
#include <DxFrameResource.h>
#include <RenderContext.h>
#include <DxCommandContext.h>
#include <PixProfiler.h>
#include <DxBuffer.h>
#include <DxTexture.h>
#include <DxUtils.h>
#include <DxDeviceGlobal.h>
#include <DxRootSignatureBuilder.h>
#include <DxShaderCompilerGlobal.h>
#include <DxDescriptorHeapGlobal.h>
#include <InputGlobal.h>

namespace
{
constexpr uint32_t kThreadGroupSizeX = 8;
constexpr uint32_t kThreadGroupSizeY = 8;
constexpr uint32_t kRestirFinalDebugViewCount = 9;

} // namespace

void RestirFinalEvaluationPass::Initialize()
{
	CreatePipeline();
	m_initialized = nullptr != m_rootSignature.Get() && m_pipelineState.IsValid();
}

void RestirFinalEvaluationPass::Release()
{
	m_pipelineState.Reset();
	m_rootSignature.Reset();
	m_initialized = false;
}

void RestirFinalEvaluationPass::DeclareRenderData(RenderContext* renderContext)
{
	if (nullptr == renderContext)
	{
		return;
	}

	renderContext->DeclareAccess<RestirFinalReservoirRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<RestirCandidateRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<RaytracingOutputRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
}

void RestirFinalEvaluationPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"RestirFinalEvaluationPass.Execute");

	if (!m_initialized || nullptr == frame || nullptr == renderContext)
	{
		return;
	}

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F10))
	{
		m_debugView = (m_debugView + 1u) % kRestirFinalDebugViewCount;
	}

	auto* finalReservoirData = renderContext->Get<RestirFinalReservoirRenderData>();
	auto* candidateData = renderContext->Get<RestirCandidateRenderData>();
	auto* outputData = renderContext->Get<RaytracingOutputRenderData>();
	if (outputData)
	{
		outputData->bypassToneMap = false;
	}
	if (nullptr == finalReservoirData || nullptr == outputData || nullptr == finalReservoirData->reservoirBuffer ||
		nullptr == candidateData || nullptr == candidateData->primaryHitBuffer || nullptr == outputData->outputTexture)
	{
		return;
	}

	auto* reservoirBuffer = finalReservoirData->reservoirBuffer.get();
	auto* primaryHitBuffer = candidateData->primaryHitBuffer.get();
	auto* outputTexture = outputData->outputTexture.get();
	if (!reservoirBuffer->HasSRV() || !primaryHitBuffer->HasSRV() || !outputTexture->HasUAV(0))
	{
		return;
	}

	const uint32_t width = outputTexture->GetWidth();
	const uint32_t height = outputTexture->GetHeight();
	if (0 == width || 0 == height)
	{
		return;
	}
	outputData->bypassToneMap = 0u != m_debugView;

	auto&			 context = *frame->GetMainContext();
	auto*			 cmdList = context.CommandList();
	DxScopedGpuEvent gpuEvent(context, L"RestirFinalEvaluation");

	DxUtils::TransitionResourceIfNeeded(cmdList, reservoirBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(cmdList, primaryHitBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(cmdList, outputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cmdList->SetPipelineState(m_pipelineState.Get());
	cmdList->SetComputeRootSignature(m_rootSignature.Get());

	auto&				  descHeap = GLOBAL(DxDescriptorHeapGlobal);
	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);

	struct Constants
	{
		uint32_t width;
		uint32_t height;
		uint32_t debugView;
		uint32_t pad1;
	};
	Constants constants = {width, height, m_debugView, 0u};

	cmdList->SetComputeRoot32BitConstants(0, 4, &constants, 0);
	cmdList->SetComputeRootDescriptorTable(1, descHeap.GetGPUHandle(reservoirBuffer->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(2, descHeap.GetGPUHandle(primaryHitBuffer->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(3, descHeap.GetGPUHandle(outputTexture->GetUAVIndex(0)));

	cmdList->Dispatch(
		(width + kThreadGroupSizeX - 1u) / kThreadGroupSizeX, (height + kThreadGroupSizeY - 1u) / kThreadGroupSizeY, 1
	);

	auto barrier = DxUtils::CreateUAVBarrier(outputTexture->GetResource());
	cmdList->ResourceBarrier(1, &barrier);
}

void RestirFinalEvaluationPass::OnResize(uint32_t width, uint32_t height) {}

void RestirFinalEvaluationPass::CreatePipeline()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& shaderCompiler = GLOBAL(DxShaderCompilerGlobal);

	auto csBlob = shaderCompiler.CompileShaderFromFile(
		L"RestirFinalEvaluation", L"Resource/Shader/RestirFinalEvaluation.hlsl", "CSMain", "cs_6_6"
	);
	if (!csBlob)
	{
		GRAPHICS_LOG_FMT("[RestirFinalEvaluationPass] ERROR: Failed to compile RestirFinalEvaluation.hlsl\n");
		return;
	}

	DxRootSignatureBuilder rootSignatureBuilder;
	m_rootSignature = rootSignatureBuilder.Add32BitConstants(4, 0)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
						  .Build(device.GetDevice(), "RestirFinalEvaluation_RootSignature");

	m_pipelineState.CreateCompute(device.GetDevice(), m_rootSignature.Get(), csBlob.Get(), "RestirFinalEvaluation_PSO");
}
