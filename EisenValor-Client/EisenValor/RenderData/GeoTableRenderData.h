#pragma once
#include "RenderDataPolicy.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"

class GeoTableRenderData : public RenderDataBase<GeoTableRenderData>
{
public:
	GeoTableRenderData() = default;
	virtual ~GeoTableRenderData() override = default;

	void Release() override
	{
		syncBuffer = RenderDataSync<GeoInfo>();
	}

	RenderDataSync<GeoInfo> syncBuffer;
};
