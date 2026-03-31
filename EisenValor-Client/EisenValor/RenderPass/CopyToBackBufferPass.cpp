#include "stdafxClient.h"
#include "CopyToBackBufferPass.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include <DxFrameResource.h>
#include <RenderContext.h>
#include <DxSwapChain.h>
#include <DxCommandContext.h>
#include <DxTexture.h>
#include <DxUtils.h>

CopyToBackBufferPass::CopyToBackBufferPass(DxSwapChain* swapChain)
	: m_swapChain(swapChain)
{
}

void CopyToBackBufferPass::Initialize()
{
}

void CopyToBackBufferPass::Release()
{
}

void CopyToBackBufferPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!m_swapChain || !renderContext)
	{
		return;
	}

	auto outputData = renderContext->GetData<RaytracingOutputRenderData>();
	if (!outputData || !outputData->outputTexture)
	{
		return;
	}

	auto* srcTexture = outputData->outputTexture.get();

	if (srcTexture->GetWidth() != m_swapChain->GetWidth() || srcTexture->GetHeight() != m_swapChain->GetHeight())
	{
		DEBUG_LOG_FMT(
			"[CopyToBackBufferPass] ERROR: Size mismatch! Src: {}x{}, BackBuffer: {}x{}\n", srcTexture->GetWidth(),
			srcTexture->GetHeight(), m_swapChain->GetWidth(), m_swapChain->GetHeight()
		);
		return;
	}

	auto& context = *frame->GetMainContext();
	auto* cmdList = context.CommandList();
	auto* backBuffer = m_swapChain->GetCurrentBackBuffer();
	auto* srcResource = srcTexture->GetResource();
	DxScopedGpuEvent passEvent(context, L"CopyToBackBufferPass");
	
	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = DxUtils::CreateTransitionBarrier(
		srcResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE
	);
	barriers[1] = DxUtils::CreateTransitionBarrier(
		backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST
	);
	cmdList->ResourceBarrier(2, barriers);

	cmdList->CopyResource(backBuffer, srcResource);

	barriers[0] = DxUtils::CreateTransitionBarrier(
		srcResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	barriers[1] = DxUtils::CreateTransitionBarrier(
		backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT
	);
	cmdList->ResourceBarrier(2, barriers);
}

void CopyToBackBufferPass::OnResize(uint32_t width, uint32_t height)
{
}
