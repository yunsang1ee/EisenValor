#pragma once
#include <IRenderPass.h>

class DxTexture;
class DxSwapChain;

class CopyToBackBufferPass : public IRenderPass
{
public:
	CopyToBackBufferPass(DxTexture* srcTexture, DxSwapChain* swapChain);
	~CopyToBackBufferPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "CopyToBackBuffer"; }

private:
	DxTexture*	 m_srcTexture = nullptr;
	DxSwapChain* m_swapChain = nullptr;
};