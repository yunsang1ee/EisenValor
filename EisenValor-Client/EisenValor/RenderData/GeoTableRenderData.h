#pragma once
#include "IRenderData.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"

class GeoTableRenderData : public RenderDataBase<GeoTableRenderData>
{
public:
	GeoTableRenderData() = default;
	virtual ~GeoTableRenderData() override = default;

	RenderDataSync<GeoInfo> syncBuffer;
};
