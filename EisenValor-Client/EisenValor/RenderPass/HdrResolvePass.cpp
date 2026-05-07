#include "stdafxClient.h"
#include "HdrResolvePass.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include <DxFrameResource.h>
#include <RenderContext.h>
#include <DxSwapChain.h>
#include <DxCommandContext.h>
#include <PixProfiler.h>
#include <DxTexture.h>
#include <DxUtils.h>
#include <DxDeviceGlobal.h>
#include <DxShaderCompilerGlobal.h>
#include <DxDescriptorHeapGlobal.h>

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

void HdrResolvePass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"HdrResolvePass.Execute");

	if (!m_initialized || !m_swapChain || !renderContext || !m_pipelineState || !m_rootSignature)
	{
		return;
	}

	auto outputData = renderContext->GetData<RaytracingOutputRenderData>();
	if (!outputData || !outputData->outputTexture)
	{
		return;
	}

	auto* srcTexture = outputData->outputTexture.get();
	if (!srcTexture->HasSRV())
	{
		return;
	}

	if (srcTexture->GetWidth() != m_swapChain->GetWidth() || srcTexture->GetHeight() != m_swapChain->GetHeight())
	{
		DEBUG_LOG_FMT(
			"[HdrResolvePass] ERROR: Size mismatch! Src: {}x{}, BackBuffer: {}x{}\n", srcTexture->GetWidth(),
			srcTexture->GetHeight(), m_swapChain->GetWidth(), m_swapChain->GetHeight()
		);
		return;
	}

	auto&			 context = *frame->GetMainContext();
	auto*			 cmdList = context.CommandList();
	auto*			 backBuffer = m_swapChain->GetCurrentBackBuffer();
	auto*			 srcResource = srcTexture->GetResource();
	DxScopedGpuEvent passEvent(context, L"HdrResolvePass");

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

	D3D12_ROOT_PARAMETER rootParams[1] = {};
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &textureRange;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
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
