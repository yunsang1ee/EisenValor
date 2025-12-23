#pragma once
#include "Singleton.h"

class Scene;
class DxFrameResource;
class DxSwapChain;
class IRenderPass;

class DxRendererGlobal : public Singleton<DxRendererGlobal>
{
private:
	friend class Singleton<DxRendererGlobal>;

	DxRendererGlobal() = default;
	~DxRendererGlobal() override = default;

public:
	void Initialize() override;
	void Release() override;

	void AddRenderPass(const std::string& name, std::unique_ptr<IRenderPass> pass);

	void RemoveRenderPass(const std::string& name);

	IRenderPass* GetRenderPass(const std::string& name) const;

	void Render(DxFrameResource* frame, Scene* scene, DxSwapChain* swapChain);

	void OnResize(uint32_t width, uint32_t height);

	void ClearAllPasses();

private:
	struct RenderPassEntry
	{
		std::string					 name;
		std::unique_ptr<IRenderPass> pass;
	};

	std::vector<RenderPassEntry> m_renderPasses;
};
