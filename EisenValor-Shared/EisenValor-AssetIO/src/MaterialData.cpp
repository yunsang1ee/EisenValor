#include "MaterialData.h"
#include <cstring>

namespace EvAsset
{
namespace
{
constexpr uint32_t kMaterialFlagTerrainSplat = 1u << 9;
}

bool MaterialData::Deserialize(AssetFile& file)
{
	// 1. PROP (Material Properties)
	const ChunkEntry* propEntry = file.GetChunkEntry("PROP");
	if (propEntry && 1 == propEntry->version)
	{
		size_t		size = 0;
		const void* ptr = file.GetChunkDataPtr("PROP", size);
		if (nullptr != ptr && sizeof(MaterialProp) <= size)
		{
			MaterialProp prop;
			std::memcpy(&prop, ptr, sizeof(MaterialProp));

			shadingModelId = prop.shadingModelId;
			materialFlags = prop.materialFlags;
			std::memcpy(albedo, prop.albedo, sizeof(float) * 4);
			roughness = prop.roughness;
			metallic = prop.metallic;
		}
	}

	// 2. DEPS (Texture Dependencies)
	const ChunkEntry* depsEntry = file.GetChunkEntry("DEPS");
	if (depsEntry && 1 == depsEntry->version)
	{
		size_t			 size = 0;
		const std::byte* ptr = static_cast<const std::byte*>(file.GetChunkDataPtr("DEPS", size));

		constexpr size_t depsHeaderSize = 4; // DEPS Header (4 Bytes) = DependencyCount(4)
		if (nullptr != ptr && depsHeaderSize <= size)
		{
			uint32_t count = 0;
			std::memcpy(&count, ptr, sizeof(uint32_t));

			const size_t requiredSize = depsHeaderSize + (count * sizeof(MaterialDepEntry));
			if (requiredSize <= size)
			{
				dependencies.reserve(count);
				const std::byte* dataStart = ptr + depsHeaderSize;

				for (uint32_t i = 0; 0 < count && i < count; ++i)
				{
					MaterialDepEntry entry;
					std::memcpy(&entry, dataStart + (i * sizeof(MaterialDepEntry)), sizeof(MaterialDepEntry));

					Dependency dep;
					std::memcpy(dep.slotType, entry.type, 4);
					dep.textureGuid = entry.assetGuid;
					dependencies.push_back(dep);
				}
			}
		}
	}

	bool			  terrainParamsLoaded = false;
	const ChunkEntry* terrainEntry = file.GetChunkEntry("TERP");
	if (terrainEntry && 1 == terrainEntry->version)
	{
		size_t			 size = 0;
		const std::byte* ptr = static_cast<const std::byte*>(file.GetChunkDataPtr("TERP", size));
		constexpr size_t requiredSize =
			sizeof(uint32_t) + sizeof(float) * 2 + sizeof(float) * 4 * 4 + sizeof(float) * 4 * 2;
		if (nullptr != ptr && requiredSize <= size)
		{
			size_t offset = 0;
			std::memcpy(&terrainLayerCount, ptr + offset, sizeof(uint32_t));
			offset += sizeof(uint32_t);
			if (4 < terrainLayerCount)
			{
				terrainLayerCount = 4;
			}

			std::memcpy(terrainSize, ptr + offset, sizeof(float) * 2);
			offset += sizeof(float) * 2;
			std::memcpy(terrainLayerTileST, ptr + offset, sizeof(float) * 4 * 4);
			offset += sizeof(float) * 4 * 4;
			std::memcpy(terrainLayerMetallicRoughness, ptr + offset, sizeof(float) * 4 * 2);
			terrainParamsLoaded = true;
		}
	}

	if (0 != (materialFlags & kMaterialFlagTerrainSplat) && !terrainParamsLoaded)
	{
		return false;
	}

	return IsValid();
}
} // namespace EvAsset
