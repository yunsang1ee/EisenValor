#pragma once
#include "IRenderData.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"

class LightRenderData : public RenderDataBase<LightRenderData>
{
public:
	LightRenderData() = default;
	~LightRenderData() override = default;

	RenderDataSync<LocalLightGPUData> syncBuffer;
	uint32_t						  lightCount = 0;
};
