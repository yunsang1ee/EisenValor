#pragma once
#include <IRenderPass.h>
class DxSwapChain;

class HdrResolvePass : public IRenderPass
{
public:
	HdrResolvePass(DxSwapChain* swapChain);
	~HdrResolvePass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "HdrResolve"; }

private:
	void CreateToneMapPipelineState();

	DxSwapChain* m_swapChain = nullptr;

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	bool						m_initialized = false;
};
