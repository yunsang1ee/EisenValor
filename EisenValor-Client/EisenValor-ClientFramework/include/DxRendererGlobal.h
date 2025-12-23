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

	void CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height);


	DxSwapChain* GetSwapChain() const { return m_swapChain.get(); }

	uint32_t GetWidth() const { return m_swapChain->GetWidth(); }
	uint32_t GetHeight() const { return m_swapChain->GetHeight(); }
	void	 OnResize(uint32_t width, uint32_t height);

	void ClearAllPasses();

private:
	DxFeatureCaps				 m_featureCaps;
	std::unique_ptr<DxSwapChain> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	uint32_t					 m_rtvDescriptorSize = 0;

	static constexpr uint32_t								  kFrameCount = 3;
	std::array<std::unique_ptr<DxFrameResource>, kFrameCount> m_frameResources;
	uint32_t												  m_currentFrameIndex = 0;

	struct RenderPassEntry
	{
		std::string					 name;
		std::unique_ptr<IRenderPass> pass;
	};
	std::vector<RenderPassEntry> m_renderPasses;
};
