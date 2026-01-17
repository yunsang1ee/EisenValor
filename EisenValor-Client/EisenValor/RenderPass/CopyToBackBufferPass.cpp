#include "stdafxClient.h"
#include "CopyToBackBufferPass.h"
#include <DxFrameResource.h>
#include <DxSwapChain.h>
#include <DxCommandContext.h>
#include <DxTexture.h>
#include <DxUtils.h>

CopyToBackBufferPass::CopyToBackBufferPass(DxTexture* srcTexture, DxSwapChain* swapChain)
	: m_srcTexture(srcTexture), m_swapChain(swapChain)
{
}

void CopyToBackBufferPass::Initialize()
{
}

void CopyToBackBufferPass::Release()
{
}

void CopyToBackBufferPass::Execute(DxFrameResource* frame, Scene* scene)
{
	if (!m_srcTexture || !m_swapChain)
		return;

	if (m_srcTexture->GetWidth() != m_swapChain->GetWidth() || m_srcTexture->GetHeight() != m_swapChain->GetHeight())
	{
		DEBUG_LOG_FMT(
			"[CopyToBackBufferPass] ERROR: Size mismatch! Src: {}x{}, BackBuffer: {}x{}\n", m_srcTexture->GetWidth(),
			m_srcTexture->GetHeight(), m_swapChain->GetWidth(), m_swapChain->GetHeight()
		);
		return;
	}

	auto* context = frame->GetMainContext();
	auto* cmdList = context->CommandList();
	auto* backBuffer = m_swapChain->GetCurrentBackBuffer();
	auto* srcResource = m_srcTexture->GetResource();
	
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