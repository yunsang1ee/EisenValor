#pragma once
#include <IRenderPass.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include "RenderData/RestirCandidateRenderData.h"

class RenderContext;

class RestirCandidatePass : public IRenderPass
{
public:
	bool ShouldExecute(const RenderContext* renderContext) const override;
	void OnSkipped(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	RestirCandidatePass(uint32_t width, uint32_t height);
	void				   Initialize() override;
	void				   Release() override;
	void				   OnResize(uint32_t width, uint32_t height) override;
	void				   DeclareRenderData(RenderContext* renderContext) override;
	void				   Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	RenderResolutionDomain GetResolutionDomain() const override { return RenderResolutionDomain::Render; }
	const char*			   GetName() const override { return "RestirCandidate"; }

private:
	void PublishInvalidCandidate(DxFrameResource* frame, RenderContext* renderContext);

	std::unique_ptr<DxRtPipelineState>	 m_pipeline;
	std::unique_ptr<DxRtShaderTable>	 m_shaderTable;
	Transient<RestirCandidateRenderData> m_restirCandidateData;
	uint32_t							 m_width = 0;
	uint32_t							 m_height = 0;
	uint64_t							 m_lastProfileRevision = ~0ull;
	bool								 m_resourcesReady = false;
};
