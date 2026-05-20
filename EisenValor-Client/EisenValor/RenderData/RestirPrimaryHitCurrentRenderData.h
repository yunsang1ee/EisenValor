#pragma once
#include "RenderDataPolicy.h"
#include "DxBuffer.h"
#include <memory>

class RestirPrimaryHitCurrentRenderData : public RenderDataBase<RestirPrimaryHitCurrentRenderData>
{
public:
	RestirPrimaryHitCurrentRenderData() = default;
	virtual ~RestirPrimaryHitCurrentRenderData() override = default;

	void Release() override
	{
		primaryHitBuffer.reset();
	}

	std::shared_ptr<DxBuffer> primaryHitBuffer;
};
