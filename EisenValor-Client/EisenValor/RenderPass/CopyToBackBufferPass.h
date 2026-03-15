#pragma once
#include <IRenderPass.h>
class DxSwapChain;

class CopyToBackBufferPass : public IRenderPass
{
public:
	CopyToBackBufferPass(DxSwapChain* swapChain);
	~CopyToBackBufferPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "CopyToBackBuffer"; }

private:
	DxSwapChain* m_swapChain = nullptr;
};