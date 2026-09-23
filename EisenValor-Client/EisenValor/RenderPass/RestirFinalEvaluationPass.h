#pragma once
#include <DxPipelineState.h>
#include <IRenderPass.h>
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RestirHdrCapture.h"
#endif

class RestirFinalEvaluationPass : public IRenderPass
{
public:
	RestirFinalEvaluationPass() = default;
	~RestirFinalEvaluationPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		DeclareRenderData(RenderContext* renderContext) override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "RestirFinalEvaluation"; }

private:
	void CreatePipeline();

private:
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	RestirHdrCapture m_hdrCapture;
#endif
	ComPtr<ID3D12RootSignature> m_rootSignature;
	DxPipelineState				m_pipelineState;
	bool						m_initialized = false;
};
