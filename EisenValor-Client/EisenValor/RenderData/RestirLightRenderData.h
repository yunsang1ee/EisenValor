#pragma once
#include "RenderDataPolicy.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"
#include <cstdint>

class RestirLightRenderData : public RenderDataBase<RestirLightRenderData>
{
public:
	RestirLightRenderData() = default;
	virtual ~RestirLightRenderData() override = default;

	void Release() override
	{
		emissiveLightSync = RenderDataSync<RestirEmissiveLightData>();
		emissiveLightCount = 0;
		emissiveLightWeightSum = 0.0f;
	}

	RenderDataSync<RestirEmissiveLightData> emissiveLightSync;
	uint32_t								emissiveLightCount = 0;
	float									emissiveLightWeightSum = 0.0f;
};
