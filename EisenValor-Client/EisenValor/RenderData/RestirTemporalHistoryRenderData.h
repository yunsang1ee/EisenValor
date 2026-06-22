#pragma once
#include "RenderDataPolicy.h"
#include "DxBuffer.h"
#include <memory>

class RestirTemporalHistoryRenderData : public RenderDataBase<RestirTemporalHistoryRenderData>
{
public:
	RestirTemporalHistoryRenderData() = default;
	virtual ~RestirTemporalHistoryRenderData() override = default;

	void Release() override
	{
		primaryHitBuffer.reset();
		reservoirBuffer.reset();
	}

	std::shared_ptr<DxBuffer> primaryHitBuffer;
	std::shared_ptr<DxBuffer> reservoirBuffer;
};
