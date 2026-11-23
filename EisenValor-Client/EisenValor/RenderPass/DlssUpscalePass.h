#pragma once

#include "RenderData/DlssOutputRenderData.h"

#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RestirHdrCapture.h"
#endif
#include <IRenderPass.h>
#include <RenderDataPolicy.h>

class DlssUpscalePass : public IRenderPass
{
public:
	DlssUpscalePass(uint32_t displayWidth, uint32_t displayHeight);
	~DlssUpscalePass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		DeclareRenderData(RenderContext* renderContext) override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "DLSS"; }

private:
	void CreateOutputResources(uint32_t width, uint32_t height);
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	RestirHdrCapture m_rrCapture;
	uint64_t		 m_seenRrCaptureRequest = 0;
#endif

	FrameBuffered<DlssOutputRenderData, 3> m_outputData;
	uint32_t							   m_displayWidth = 0;
	uint32_t							   m_displayHeight = 0;
	uint32_t							   m_streamlineFrameIndex = 0;
	bool								   m_initialized = false;
	bool								   m_resetHistory = true;
};
