#pragma once
#include "RenderDataPolicy.h"
#include "DxBuffer.h"
#include <cstdint>
#include <memory>

enum class RestirFinalReservoirSource : uint32_t
{
	None = 0,
	Initial,
	Temporal,
	Spatial
};

class RestirFinalReservoirRenderData : public RenderDataBase<RestirFinalReservoirRenderData>
{
public:
	RestirFinalReservoirRenderData() = default;
	virtual ~RestirFinalReservoirRenderData() override = default;

	void Release() override
	{
		reservoirBuffer.reset();
		source = RestirFinalReservoirSource::None;
	}

	std::shared_ptr<DxBuffer> reservoirBuffer;
	RestirFinalReservoirSource source = RestirFinalReservoirSource::None;
};
