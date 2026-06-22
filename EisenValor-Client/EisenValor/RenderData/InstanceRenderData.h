#pragma once
#include "RenderDataPolicy.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"

class InstanceRenderData : public RenderDataBase<InstanceRenderData>
{
public:
	InstanceRenderData() = default;
	virtual ~InstanceRenderData() override = default;

	void Release() override
	{
		syncBuffer = RenderDataSync<InstanceData>();
		idToInstanceIndexSync = RenderDataSync<uint32_t>();
		tlasDescriptorIndex = 0;
		tlasAddress = 0;
	}

	RenderDataSync<InstanceData> syncBuffer;
	RenderDataSync<uint32_t>	 idToInstanceIndexSync;

	uint32_t				  tlasDescriptorIndex = 0;
	D3D12_GPU_VIRTUAL_ADDRESS tlasAddress = 0;
};
