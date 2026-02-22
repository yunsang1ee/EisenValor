#pragma once
#include "Singleton.h"
#include "DxFeatureCaps.h"
#include "RenderContext.h"

class Scene;
class DxFrameResource;
class DxSwapChain;
class IRenderPass;

// 우선순위 배정
enum class RenderPassPriority : int32_t
{
	High = 0,
	World = 1000,
	PostProcess = 2000,
	UI = 3000
};

class DxRendererGlobal : public Singleton<DxRendererGlobal>
{
private:
	friend class Singleton<DxRendererGlobal>;

	DxRendererGlobal();
	~DxRendererGlobal() override;

public:
	void Initialize() override;
	void Release() override;

	void AddRenderPass(
		const std::string&			 name,
		std::unique_ptr<IRenderPass> pass,
		RenderPassPriority			 priority = RenderPassPriority::World
	);

	void		 RemoveRenderPass(const std::string& name);
	IRenderPass* GetRenderPass(const std::string& name) const;

	void BeginFrame();
	void Render(Scene* scene);
	void EndFrame();

	void		 CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height);
	DxSwapChain* GetSwapChain() const { return m_swapChain.get(); }
	void		 OnResize(uint32_t width, uint32_t height);

	void ClearAllPasses();

	DxFrameResource*	 GetCurrentFrame() const;
	const RenderContext* GetRenderContext() const { return &m_renderContext; }
	RenderContext*		 GetRenderContext() { return &m_renderContext; }

private:
	bool						 m_isInitialized = false;
	DxFeatureCaps				 m_featureCaps;
	std::unique_ptr<DxSwapChain> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	uint32_t					 m_rtvDescriptorSize = 0;

	static constexpr uint32_t								  kFrameCount = 3;
	uint32_t												  m_currentFrameIndex = 0;
	std::array<std::unique_ptr<DxFrameResource>, kFrameCount> m_frameResources;
	RenderContext											  m_renderContext;

	struct RenderPassEntry
	{
		std::string					 name;
		std::unique_ptr<IRenderPass> pass;
		RenderPassPriority			 priority;
	};
	std::vector<RenderPassEntry> m_renderPasses;
	// 더티 플래그
	bool m_renderPassesDirty = false;
};
