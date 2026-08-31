#include "stdafxClient.h"
#include "EffectRenderPass.h"
#include "RenderData/EffectRenderData.h"
#include <PixProfiler.h>

void EffectRenderPass::Initialize()
{
	m_initialized = true;
}

void EffectRenderPass::Release()
{
	m_particleBuffer.reset();
	m_initialized = false;
}

// 
void EffectRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"EffectRenderPass.Execute");
	(void)frame;
	(void)scene;
	(void)renderContext;

	if (!m_initialized)
	{
		return;
	}

	auto events = EffectEventQueue::Consume();
	(void)events;
}

void EffectRenderPass::OnResize(uint32_t width, uint32_t height)
{
	m_screenWidth = width;
	m_screenHeight = height;
}

void EffectRenderPass::CreatePipelineState()
{
}

void EffectRenderPass::CreateParticleBuffer()
{
}
