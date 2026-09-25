#pragma once
#include <IRenderPass.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <memory>

class ReferencePathTracingPass : public IRenderPass
{
public:
	void				   Initialize() override;
	void				   Release() override;
	void				   DeclareRenderData(RenderContext* renderContext) override;
	void				   Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void				   OnResize(uint32_t width, uint32_t height) override {}
	RenderResolutionDomain GetResolutionDomain() const override { return RenderResolutionDomain::Render; }
	const char*			   GetName() const override { return "ReferencePT"; }

private:
	std::unique_ptr<DxRtPipelineState> m_pipeline;
	std::unique_ptr<DxRtShaderTable>   m_shaderTable;
};
