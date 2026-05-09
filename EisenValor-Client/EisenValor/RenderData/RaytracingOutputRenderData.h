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
	}

	std::shared_ptr<DxTexture> outputTexture;
};
