#pragma once
#include "IRenderData.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"

class InstanceRenderData : public RenderDataBase<InstanceRenderData>
{
public:
	InstanceRenderData() = default;
	virtual ~InstanceRenderData() override = default;

	RenderDataSync<InstanceData> syncBuffer;

	uint32_t tlasDescriptorIndex = 0;
	D3D12_GPU_VIRTUAL_ADDRESS tlasAddress = 0;
};
