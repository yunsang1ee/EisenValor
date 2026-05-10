#pragma once
#include "RenderDataPolicy.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"
#include "AssetFormat.h"
#include <unordered_map>

class MaterialRenderData : public RenderDataBase<MaterialRenderData>
{
public:
	MaterialRenderData() = default;
	virtual ~MaterialRenderData() override = default;

	void Release() override
	{
		syncBuffer = RenderDataSync<MaterialGPUData>();
		terrainSurfaceSyncBuffer = RenderDataSync<TerrainSurfaceGPUData>();
		materialToIndex.clear();
	}

	RenderDataSync<MaterialGPUData> syncBuffer;
	RenderDataSync<TerrainSurfaceGPUData> terrainSurfaceSyncBuffer;

	std::unordered_map<EvAsset::Guid, uint32_t, EvAsset::GuidHash> materialToIndex;

	uint32_t GetIndex(const EvAsset::Guid& guid) const
	{
		auto it = materialToIndex.find(guid);
		return (it != materialToIndex.end()) ? it->second : ~0u;
	}
};
