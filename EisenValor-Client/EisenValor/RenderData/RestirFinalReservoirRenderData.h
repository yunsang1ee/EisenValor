#pragma once
#include "RenderDataPolicy.h"
#include "DxBuffer.h"
#include <memory>

class RestirFinalReservoirRenderData : public RenderDataBase<RestirFinalReservoirRenderData>
{
public:
	RestirFinalReservoirRenderData() = default;
	virtual ~RestirFinalReservoirRenderData() override = default;

	void Release() override { reservoirBuffer.reset(); }

	std::shared_ptr<DxBuffer> reservoirBuffer;
};
