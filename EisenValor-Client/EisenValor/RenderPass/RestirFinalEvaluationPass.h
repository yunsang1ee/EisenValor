#pragma once
#include <DxPipelineState.h>
#include <IRenderPass.h>

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
	ComPtr<ID3D12RootSignature> m_rootSignature;
	DxPipelineState				m_pipelineState;
	bool						m_initialized = false;
	uint32_t					m_debugView = 0;
};
