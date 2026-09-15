#pragma once
#include "RenderDataPolicy.h"
#include "DxTexture.h"
#include <memory>

class RaytracingOutputRenderData : public RenderDataBase<RaytracingOutputRenderData>
{
public:
	RaytracingOutputRenderData() = default;
	virtual ~RaytracingOutputRenderData() override = default;

	void Release() override
	{
		outputTexture.reset();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
		bypassDlss = false;
#endif
		bypassToneMap = false;
	}

	std::shared_ptr<DxTexture> outputTexture;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	bool					   bypassDlss = false;
#endif
	bool					   bypassToneMap = false;
};
