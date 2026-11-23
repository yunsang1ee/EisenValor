#pragma once
#include <IRenderPass.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <DirectXMath.h>
#include <memory>

class DxBuffer;

class ReferencePathTracingPass : public IRenderPass
{
public:
	bool				   ShouldExecute(const RenderContext* renderContext) const override;
	void				   Initialize() override;
	void				   Release() override;
	void				   DeclareRenderData(RenderContext* renderContext) override;
	void				   Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void				   OnSkipped(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void				   OnResize(uint32_t width, uint32_t height) override;
	RenderResolutionDomain GetResolutionDomain() const override { return RenderResolutionDomain::Render; }
	const char*			   GetName() const override { return "ReferencePT"; }

	void ResetAccumulation();

private:
	void CreateAccumulationBuffer(uint32_t width, uint32_t height);

	static constexpr uint32_t		   kMaxAccumulationFrames = 1024;
	std::unique_ptr<DxRtPipelineState> m_pipeline;
	std::unique_ptr<DxRtShaderTable>   m_shaderTable;
	std::shared_ptr<DxBuffer>		   m_accumulationBuffer;
	DirectX::XMFLOAT4X4				   m_accumulationView = {};
	DirectX::XMFLOAT4X4				   m_accumulationProjection = {};
	const Scene*					   m_previousScene = nullptr;
	uint64_t						   m_historySignature = 0;
	uint64_t						   m_resetRequest = 0;
	uint32_t						   m_accumulationFrames = 0;
	uint32_t						   m_width = 0;
	uint32_t						   m_height = 0;
	bool							   m_previousPhysicalMode = false;
	bool							   m_previousDayEnvironment = false;
};
