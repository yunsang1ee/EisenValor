#include "stdafxClient.h"
#include "RestirFinalEvaluationPass.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include "RenderData/RestirCandidateRenderData.h"
#include "RenderData/RestirFinalReservoirRenderData.h"
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RestirDebugGlobal.h"
#include <CameraRenderData.h>
#endif
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

namespace
{
constexpr uint32_t kThreadGroupSizeX = 8;
constexpr uint32_t kThreadGroupSizeY = 8;

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
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	renderContext->DeclareAccess<RestirCandidateRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<CameraRenderData>(GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read);
#endif
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

	auto* finalReservoirData = renderContext->Get<RestirFinalReservoirRenderData>();
	auto* outputData = renderContext->Get<RaytracingOutputRenderData>();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	auto*		   candidateData = renderContext->Get<RestirCandidateRenderData>();
	auto*		   cameraData = renderContext->Get<CameraRenderData>();
	auto&		   debug = GLOBAL(RestirDebugGlobal);
	const bool	   debugOverrideActive = debug.IsOverrideActive();
	const uint32_t debugView = debugOverrideActive ? static_cast<uint32_t>(debug.GetView()) : 0u;
	if (outputData)
	{
		outputData->bypassDlss = debugOverrideActive && debug.BypassDlss();
		outputData->bypassToneMap = debugOverrideActive && debug.BypassToneMap();
	}
	if (debugOverrideActive && RestirDebugSource::ReferencePathTracingRaw == debug.GetSource())
	{
		return;
	}
	if (nullptr == candidateData || nullptr == candidateData->motionVectorTexture ||
		nullptr == candidateData->linearDepthTexture || nullptr == candidateData->diffuseAlbedoTexture ||
		nullptr == candidateData->specularAlbedoTexture || nullptr == candidateData->normalRoughnessTexture)
	{
		return;
	}
#endif
	if (nullptr == finalReservoirData || nullptr == outputData || nullptr == finalReservoirData->reservoirBuffer ||
		nullptr == outputData->outputTexture)
	{
		return;
	}

	auto* reservoirBuffer = finalReservoirData->reservoirBuffer.get();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	auto* motionVectorTexture = candidateData->motionVectorTexture.get();
	auto* linearDepthTexture = candidateData->linearDepthTexture.get();
	auto* diffuseAlbedoTexture = candidateData->diffuseAlbedoTexture.get();
	auto* specularAlbedoTexture = candidateData->specularAlbedoTexture.get();
	auto* normalRoughnessTexture = candidateData->normalRoughnessTexture.get();
#endif
	auto* outputTexture = outputData->outputTexture.get();
	if (!reservoirBuffer->HasSRV() || !outputTexture->HasUAV(0))
	{
		return;
	}
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	if (!motionVectorTexture->HasSRV() || !linearDepthTexture->HasSRV() || !diffuseAlbedoTexture->HasSRV() ||
		!specularAlbedoTexture->HasSRV() || !normalRoughnessTexture->HasSRV())
	{
		return;
	}
#endif

	const uint32_t width = outputTexture->GetWidth();
	const uint32_t height = outputTexture->GetHeight();
	if (0 == width || 0 == height)
	{
		return;
	}
	auto&			 context = *frame->GetMainContext();
	auto*			 cmdList = context.CommandList();
	DxScopedGpuEvent gpuEvent(context, L"RestirFinalEvaluation");

	DxUtils::TransitionResourceIfNeeded(cmdList, reservoirBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	DxUtils::TransitionResourceIfNeeded(cmdList, motionVectorTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(cmdList, linearDepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(cmdList, diffuseAlbedoTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(cmdList, specularAlbedoTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(
		cmdList, normalRoughnessTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
#endif
	DxUtils::TransitionResourceIfNeeded(cmdList, outputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cmdList->SetPipelineState(m_pipelineState.Get());
	cmdList->SetComputeRootSignature(m_rootSignature.Get());

	auto&				  descHeap = GLOBAL(DxDescriptorHeapGlobal);
	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);

#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	struct Constants
	{
		uint32_t width;
		uint32_t height;
		uint32_t debugView;
		float	 cameraFarZ;
	};
	Constants constants = {width, height, debugView, cameraData ? cameraData->farZ : 1000.0f};
	cmdList->SetComputeRoot32BitConstants(0, 4, &constants, 0);
#else
	struct Constants
	{
		uint32_t width;
		uint32_t height;
	};
	Constants constants = {width, height};
	cmdList->SetComputeRoot32BitConstants(0, 2, &constants, 0);
#endif
	cmdList->SetComputeRootDescriptorTable(1, descHeap.GetGPUHandle(reservoirBuffer->GetSRVIndex()));
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	cmdList->SetComputeRootDescriptorTable(2, descHeap.GetGPUHandle(motionVectorTexture->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(3, descHeap.GetGPUHandle(linearDepthTexture->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(4, descHeap.GetGPUHandle(diffuseAlbedoTexture->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(5, descHeap.GetGPUHandle(specularAlbedoTexture->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(6, descHeap.GetGPUHandle(normalRoughnessTexture->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(7, descHeap.GetGPUHandle(outputTexture->GetUAVIndex(0)));
#else
	cmdList->SetComputeRootDescriptorTable(2, descHeap.GetGPUHandle(outputTexture->GetUAVIndex(0)));
#endif

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

#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	const std::pair<std::wstring, std::wstring> shaderDefines[] = {{L"RESTIR_ENABLE_DEBUG_VIEWS", L"1"}};
	auto										csBlob = shaderCompiler.CompileShaderFromFile(
		   L"RestirFinalEvaluation", L"Resource/Shader/RestirFinalEvaluation.hlsl", "CSMain", "cs_6_6", shaderDefines
	   );
#else
	auto csBlob = shaderCompiler.CompileShaderFromFile(
		L"RestirFinalEvaluation", L"Resource/Shader/RestirFinalEvaluation.hlsl", "CSMain", "cs_6_6"
	);
#endif
	if (!csBlob)
	{
		GRAPHICS_LOG_FMT("[RestirFinalEvaluationPass] ERROR: Failed to compile RestirFinalEvaluation.hlsl\n");
		return;
	}

	DxRootSignatureBuilder rootSignatureBuilder;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	rootSignatureBuilder.Add32BitConstants(4, 0);
#else
	rootSignatureBuilder.Add32BitConstants(2, 0);
#endif
	rootSignatureBuilder.AddDescriptorTable().AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	rootSignatureBuilder.AddDescriptorTable().AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	rootSignatureBuilder.AddDescriptorTable().AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
	rootSignatureBuilder.AddDescriptorTable().AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
	rootSignatureBuilder.AddDescriptorTable().AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
	rootSignatureBuilder.AddDescriptorTable().AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
#endif
	rootSignatureBuilder.AddDescriptorTable().AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	m_rootSignature = rootSignatureBuilder.Build(device.GetDevice(), "RestirFinalEvaluation_RootSignature");

	m_pipelineState.CreateCompute(device.GetDevice(), m_rootSignature.Get(), csBlob.Get(), "RestirFinalEvaluation_PSO");
}
