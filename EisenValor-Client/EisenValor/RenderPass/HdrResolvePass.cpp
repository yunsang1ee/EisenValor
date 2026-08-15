#include "stdafxClient.h"
#include "HdrResolvePass.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include "RenderData/DlssOutputRenderData.h"
#include <DxFrameResource.h>
#include <FrameRenderData.h>
#include <RenderContext.h>
#include <DxSwapChain.h>
#include <DxCommandContext.h>
#include <PixProfiler.h>
#include <DxTexture.h>
#include <DxUtils.h>
#include <DxDeviceGlobal.h>
#include <DxShaderCompilerGlobal.h>
#include <DxDescriptorHeapGlobal.h>

#include <cstdio>

namespace
{
void AppendMissingInputFlag(char* buffer, size_t bufferSize, uint32_t mask, uint32_t flag, const char* name)
{
	if (0u == (mask & flag))
	{
		return;
	}

	if ('\0' != buffer[0])
	{
		strncat_s(buffer, bufferSize, "|", _TRUNCATE);
	}
	strncat_s(buffer, bufferSize, name, _TRUNCATE);
}

const char* ToMissingInputString(uint32_t mask, char* buffer, size_t bufferSize)
{
	if (0u == mask)
	{
		return "-";
	}

	buffer[0] = '\0';
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoRaytracingOutput, "NoRaytracingOutput");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoCandidateData, "NoCandidateData");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoCameraData, "NoCameraData");
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::BypassRequested, "BypassRequested");
#endif
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::CandidateInvalid, "CandidateInvalid");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoColorInput, "NoColorInput");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoColorOutput, "NoColorOutput");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoDepth, "NoDepth");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoMotionVectors, "NoMotionVectors");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoDiffuseAlbedo, "NoDiffuseAlbedo");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoSpecularAlbedo, "NoSpecularAlbedo");
	AppendMissingInputFlag(buffer, bufferSize, mask, DlssMissingInputMask::NoNormalRoughness, "NoNormalRoughness");
	return buffer;
}

const char* ToDebugString(DlssOutputStatus status)
{
	switch (status)
	{
	case DlssOutputStatus::NotRun:
		return "NotRun";
	case DlssOutputStatus::Disabled:
		return "Disabled";
	case DlssOutputStatus::MissingInput:
		return "MissingInput";
	case DlssOutputStatus::MissingGuideInput:
		return "MissingGuideInput";
	case DlssOutputStatus::EvaluateFailed:
		return "EvaluateFailed";
	case DlssOutputStatus::Valid:
		return "Valid";
	case DlssOutputStatus::WarmingUp:
		return "WarmingUp";
	default:
		return "Unknown";
	}
}

void OutputDlssResolveState(
	bool usingDlss, const DlssOutputRenderData* dlssOutputData, const DxTexture* srcTexture, float frameMs
)
{
	static bool				s_hasLastState = false;
	static bool				s_lastUsingDlss = false;
	static DlssOutputStatus s_lastStatus = DlssOutputStatus::NotRun;
	static ULONGLONG		s_lastResolveTickMs = 0u;

	const DlssOutputStatus status = dlssOutputData ? dlssOutputData->status : DlssOutputStatus::NotRun;
	const uint32_t		   width = srcTexture ? srcTexture->GetWidth() : 0u;
	const uint32_t		   height = srcTexture ? srcTexture->GetHeight() : 0u;
	const uint32_t		   valid = dlssOutputData && dlssOutputData->validThisFrame ? 1u : 0u;
	const uint32_t		   rr = dlssOutputData && dlssOutputData->usedRayReconstruction ? 1u : 0u;
	const uint32_t missingInputMask = dlssOutputData ? dlssOutputData->missingInputMask : DlssMissingInputMask::None;

	static uint32_t	 s_lastMissingInputMask = UINT32_MAX;
	static uint32_t	 s_lastWidth = 0u;
	static uint32_t	 s_lastHeight = 0u;
	static ULONGLONG s_lastLogTickMs = 0u;

	const ULONGLONG nowMs = GetTickCount64();
	const double	resolveGapMs = 0u == s_lastResolveTickMs ? 0.0 : static_cast<double>(nowMs - s_lastResolveTickMs);
	s_lastResolveTickMs = nowMs;

	const bool changed = !s_hasLastState || s_lastUsingDlss != usingDlss || s_lastStatus != status ||
						 s_lastMissingInputMask != missingInputMask || s_lastWidth != width || s_lastHeight != height;
	if (!changed && nowMs - s_lastLogTickMs < 2000u)
	{
		return;
	}

	s_hasLastState = true;
	s_lastUsingDlss = usingDlss;
	s_lastStatus = status;
	s_lastMissingInputMask = missingInputMask;
	s_lastWidth = width;
	s_lastHeight = height;
	s_lastLogTickMs = nowMs;

	char missingInputText[256] = {};
	char message[512] = {};
	std::snprintf(
		message, sizeof(message),
		"[DLSS.Resolve] source=%s status=%s valid=%u rr=%u src=%ux%u frameMs=%.2f resolveGapMs=%.2f "
		"missing=0x%X(%s)\n",
		usingDlss ? "DLSS" : "RawFallback", ToDebugString(status), valid, rr, width, height, frameMs, resolveGapMs,
		missingInputMask, ToMissingInputString(missingInputMask, missingInputText, sizeof(missingInputText))
	);
#if defined(_DEBUG) || defined(ENABLE_DEBUG_LOG)
	DEBUG_LOG_FMT("{}", message);
#else
	OutputDebugStringA(message);
#endif
}
} // namespace

HdrResolvePass::HdrResolvePass(DxSwapChain* swapChain) : m_swapChain(swapChain) {}

void HdrResolvePass::Initialize()
{
	CreateToneMapPipelineState();
	m_initialized = true;
}

void HdrResolvePass::Release()
{
	m_pipelineState.Reset();
	m_rootSignature.Reset();
	m_initialized = false;
}

void HdrResolvePass::DeclareRenderData(RenderContext* renderContext)
{
	if (nullptr == renderContext)
	{
		return;
	}

	renderContext->DeclareAccess<RaytracingOutputRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<DlssOutputRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<FrameRenderData>(GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read);
}

void HdrResolvePass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"HdrResolvePass.Execute");

	if (!m_initialized || !m_swapChain || !renderContext || !m_pipelineState || !m_rootSignature)
	{
		return;
	}

	auto* outputData = renderContext->Get<RaytracingOutputRenderData>();
	if (!outputData || !outputData->outputTexture)
	{
		return;
	}

	auto*	   dlssOutputData = renderContext->Get<DlssOutputRenderData>();
	auto*	   frameData = renderContext->Get<FrameRenderData>();
	auto*	   srcTexture = outputData->outputTexture.get();
	bool	   bypassToneMap = outputData->bypassToneMap;
	const bool usingDlss = dlssOutputData && dlssOutputData->validThisFrame && dlssOutputData->outputTexture;
	if (usingDlss)
	{
		srcTexture = dlssOutputData->outputTexture.get();
		bypassToneMap = false;
	}
	const float frameMs = frameData ? frameData->deltaTime * 1000.0f : -1.0f;
	OutputDlssResolveState(usingDlss, dlssOutputData, srcTexture, frameMs);
	if (!srcTexture->HasSRV())
	{
		return;
	}

	auto& context = *frame->GetMainContext();
	auto* cmdList = context.CommandList();
	auto* backBuffer = m_swapChain->GetCurrentBackBuffer();
	auto* srcResource = srcTexture->GetResource();

	D3D12_RESOURCE_STATES  srcBefore = srcTexture->GetCurrentState();
	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = DxUtils::CreateTransitionBarrier(srcResource, srcBefore, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	barriers[1] =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->ResourceBarrier(2, barriers);
	srcTexture->SetState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_swapChain->GetCurrentBackBufferRTV();
	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	D3D12_VIEWPORT viewport = {
		0.0f, 0.0f, static_cast<float>(m_swapChain->GetWidth()), static_cast<float>(m_swapChain->GetHeight()),
		0.0f, 1.0f
	};
	D3D12_RECT scissorRect = {
		0, 0, static_cast<LONG>(m_swapChain->GetWidth()), static_cast<LONG>(m_swapChain->GetHeight())
	};
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	cmdList->SetPipelineState(m_pipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	auto&				  descHeap = GLOBAL(DxDescriptorHeapGlobal);
	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);
	cmdList->SetGraphicsRootDescriptorTable(0, descHeap.GetGPUHandle(srcTexture->GetSRVIndex()));
	struct Constants
	{
		uint32_t bypassToneMap;
		uint32_t pad0;
		uint32_t pad1;
		uint32_t pad2;
	};
	Constants constants = {bypassToneMap ? 1u : 0u, 0u, 0u, 0u};
	cmdList->SetGraphicsRoot32BitConstants(1, 4, &constants, 0);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);

	barriers[0] = DxUtils::CreateTransitionBarrier(
		srcResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	barriers[1] =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	cmdList->ResourceBarrier(2, barriers);
	srcTexture->SetState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void HdrResolvePass::OnResize(uint32_t width, uint32_t height) {}

void HdrResolvePass::CreateToneMapPipelineState()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& shaderCompiler = GLOBAL(DxShaderCompilerGlobal);

	auto vsBlob = shaderCompiler.CompileShaderFromFile(
		L"FullscreenToneMapVS", L"Resource/Shader/FullscreenToneMap.hlsl", "VSMain", "vs_6_6"
	);
	auto psBlob = shaderCompiler.CompileShaderFromFile(
		L"FullscreenToneMapPS", L"Resource/Shader/FullscreenToneMap.hlsl", "PSMain", "ps_6_6"
	);

	D3D12_DESCRIPTOR_RANGE textureRange = {};
	textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	textureRange.NumDescriptors = 1;
	textureRange.BaseShaderRegister = 0;
	textureRange.RegisterSpace = 0;
	textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParams[2] = {};
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &textureRange;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[1].Constants.ShaderRegister = 0;
	rootParams[1].Constants.RegisterSpace = 0;
	rootParams[1].Constants.Num32BitValues = 4;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 2;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device.GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)
	));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_swapChain ? m_swapChain->GetInfo().format : DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(device.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}
