#pragma once
#include "AssetFormat.h"
#include "DxTLAS.h"
#include "IRenderData.h"
#include "RaytracingCommon.h"
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

class Scene;

class StaticSceneRenderData : public RenderDataBase<StaticSceneRenderData>
{
public:
	struct TlasInstance
	{
		uint64_t						objectHandle = 0;
		uint64_t						meshComponentHandle = 0;
		uint64_t						blasAddress = 0;
		D3D12_RAYTRACING_INSTANCE_FLAGS flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	};

	void Release() override
	{
		scene = nullptr;
		meshRevision = 0;
		meshComponentCount = 0;
		valid = false;
		complete = false;
		pendingLoadsActive = false;
		topologyHash = 0;
		transformHash = 0;
		emissiveLightWeightSum = 0.0f;

		std::vector<InstanceData>().swap(instances);
		std::vector<MaterialGPUData>().swap(materials);
		std::vector<TerrainSurfaceGPUData>().swap(terrainSurfaces);
		std::vector<GeoInfo>().swap(geometry);
		std::vector<RestirEmissiveLightData>().swap(emissiveLights);
		std::vector<TlasInstance>().swap(tlasInstances);
		std::vector<uint32_t>().swap(instanceIdLookup);
		materialToIndex.clear();
		materialToIndex.rehash(0);
	}

	bool IsValid() const override { return valid; }

	Scene*	 scene = nullptr;
	uint64_t meshRevision = 0;
	size_t	 meshComponentCount = 0;
	bool	 valid = false;
	bool	 complete = false;
	bool	 pendingLoadsActive = false;
	uint64_t topologyHash = 0;
	uint64_t transformHash = 0;
	float	 emissiveLightWeightSum = 0.0f;

	std::vector<InstanceData>									   instances;
	std::vector<MaterialGPUData>								   materials;
	std::vector<TerrainSurfaceGPUData>							   terrainSurfaces;
	std::vector<GeoInfo>										   geometry;
	std::vector<RestirEmissiveLightData>						   emissiveLights;
	std::vector<TlasInstance>									   tlasInstances;
	std::vector<uint32_t>										   instanceIdLookup;
	std::unordered_map<EvAsset::Guid, uint32_t, EvAsset::GuidHash> materialToIndex;
};
