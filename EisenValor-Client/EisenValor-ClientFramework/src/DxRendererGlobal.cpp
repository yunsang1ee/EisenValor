#include "stdafxClientFramework.h"
#include "DxRendererGlobal.h"
#include "IRenderPass.h"
#include "Scene.h"
#include "DxFrameResource.h"
#include "DxSwapChain.h"

void DxRendererGlobal::Initialize()
{
	DEBUG_LOG_FMT("[DxRendererGlobal] Initialized\n");
}

void DxRendererGlobal::Release()
{
	ClearAllPasses();
	DEBUG_LOG_FMT("[DxRendererGlobal] Released\n");
}

void DxRendererGlobal::AddRenderPass(const std::string& name, std::unique_ptr<IRenderPass> pass)
{
	for (const auto& entry : m_renderPasses)
	{
		if (entry.name == name)
		{
			DEBUG_LOG_FMT("[DxRendererGlobal] WARNING: Pass already exists: {}\n", name);
			return;
		}
	}

	pass->Initialize();
	m_renderPasses.push_back({name, std::move(pass)});

	DEBUG_LOG_FMT("[DxRendererGlobal] Pass added: {}\n", name);
}

void DxRendererGlobal::RemoveRenderPass(const std::string& name)
{
	auto iter = std::remove_if(
		m_renderPasses.begin(), m_renderPasses.end(),
		[&name](const RenderPassEntry& entry)
		{
			if (entry.name == name)
			{
				entry.pass->Release();
				return true;
			}
			return false;
		}
	);

	if (iter != m_renderPasses.end())
	{
		m_renderPasses.erase(iter, m_renderPasses.end());
		DEBUG_LOG_FMT("[DxRendererGlobal] Pass removed: {}\n", name);
	}
}

IRenderPass* DxRendererGlobal::GetRenderPass(const std::string& name) const
{
	for (const auto& entry : m_renderPasses)
	{
		if (entry.name == name)
		{
			return entry.pass.get();
		}
	}
	return nullptr;
}

void DxRendererGlobal::Render(DxFrameResource* frame, Scene* scene, DxSwapChain* swapChain)
{
	if (!scene)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] WARNING: Scene is null!\n");
		return;
	}

	for (auto& entry : m_renderPasses)
	{
		entry.pass->Execute(frame, scene);
	}
}

void DxRendererGlobal::OnResize(uint32_t width, uint32_t height)
{
	for (auto& entry : m_renderPasses)
	{
		entry.pass->OnResize(width, height);
	}
}

void DxRendererGlobal::ClearAllPasses()
{
	for (auto& entry : m_renderPasses)
	{
		entry.pass->Release();
	}
	m_renderPasses.clear();
	DEBUG_LOG_FMT("[DxRendererGlobal] All passes cleared\n");
}