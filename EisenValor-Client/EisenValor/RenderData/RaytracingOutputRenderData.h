#pragma once
#include "IRenderData.h"
#include "DxTexture.h"
#include <memory>

class RaytracingOutputRenderData : public RenderDataBase<RaytracingOutputRenderData>
{
public:
	RaytracingOutputRenderData() = default;
	virtual ~RaytracingOutputRenderData() override = default;

	std::shared_ptr<DxTexture> outputTexture;
};
