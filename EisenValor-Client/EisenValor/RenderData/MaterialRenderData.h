#pragma once
#include "IRenderData.h"
#include "RenderDataSync.h"
#include "RaytracingCommon.h"
#include "AssetFormat.h"
#include <unordered_map>

class MaterialRenderData : public RenderDataBase<MaterialRenderData>
{
public:
	MaterialRenderData() = default;
	virtual ~MaterialRenderData() override = default;

	RenderDataSync<MaterialGPUData> syncBuffer;
	RenderDataSync<TerrainSurfaceGPUData> terrainSurfaceSyncBuffer;

	std::unordered_map<EvAsset::Guid, uint32_t, EvAsset::GuidHash> materialToIndex;
	std::unordered_map<EvAsset::Guid, uint32_t, EvAsset::GuidHash> materialToTerrainSurfaceIndex;

	uint32_t GetIndex(const EvAsset::Guid& guid) const
	{
		auto it = materialToIndex.find(guid);
		return (it != materialToIndex.end()) ? it->second : ~0u;
	}
};
