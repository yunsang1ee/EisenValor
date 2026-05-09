#pragma once
#include "RenderDataPolicy.h"

class FrameRenderData : public RenderDataBase<FrameRenderData>
{
public:
	FrameRenderData() = default;
	virtual ~FrameRenderData() override = default;

	float	 deltaTime = 0.0f;
	float	 totalTime = 0.0f;
	uint32_t frameIndex = 0;
};
