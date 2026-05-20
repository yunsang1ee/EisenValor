#pragma once
#include "RenderDataPolicy.h"
#include "DxBuffer.h"
#include <memory>

class RestirReservoirInitialRenderData : public RenderDataBase<RestirReservoirInitialRenderData>
{
public:
	RestirReservoirInitialRenderData() = default;
	virtual ~RestirReservoirInitialRenderData() override = default;

	void Release() override
	{
		reservoirBuffer.reset();
	}

	std::shared_ptr<DxBuffer> reservoirBuffer;
};
