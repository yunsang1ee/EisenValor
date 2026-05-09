#include "MaterialData.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace EvAsset
{
namespace
{
constexpr uint32_t kMaterialFlagUseAlbedoMap = 1u << 0;
constexpr uint32_t kMaterialFlagUseNormalMap = 1u << 1;
constexpr uint32_t kMaterialFlagUseOrmMap = 1u << 2;
constexpr uint32_t kMaterialFlagEmissiveMap = 1u << 6;
constexpr uint32_t kMaterialFlagTerrainSplat = 1u << 9;
constexpr uint32_t kKnownMaterialFlags = (1u << 10) - 1u;

bool IsKnownShadingModel(ShadingModel shadingModel)
{
	switch (shadingModel)
	{
	case ShadingModel::LitPbr:
	case ShadingModel::Unlit:
	case ShadingModel::ClearCoat:
	case ShadingModel::Skin:
	case ShadingModel::Custom:
		return true;
	default:
		return false;
	}
}

bool IsFiniteArray(const float* values, size_t count)
{
	return std::all_of(values, values + count, [](float value) { return std::isfinite(value); });
}

bool HasDependencySlot(const std::vector<MaterialData::Dependency>& dependencies, const char slotType[4])
{
	return std::any_of(
		dependencies.begin(), dependencies.end(),
		[slotType](const MaterialData::Dependency& dep) { return 0 == std::memcmp(dep.slotType, slotType, 4); }
	);
}
}

bool MaterialData::Deserialize(AssetFile& file)
{
	m_hasProperties = false;
	m_hasDependencies = false;
	m_hasTerrainParams = false;
	dependencies.clear();

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
			m_hasProperties = true;
		}
	}

	if (!m_hasProperties)
	{
		return false;
	}

	const ChunkEntry* emissiveEntry = file.GetChunkEntry("EMIS");
	if (emissiveEntry && 1 == emissiveEntry->version)
	{
		size_t			 size = 0;
		const std::byte* ptr = static_cast<const std::byte*>(file.GetChunkDataPtr("EMIS", size));
		constexpr size_t requiredSize = sizeof(float) * 4;
		if (nullptr != ptr && requiredSize <= size)
		{
			size_t offset = 0;
			std::memcpy(emissiveColor, ptr + offset, sizeof(float) * 3);
			offset += sizeof(float) * 3;
			std::memcpy(&emissiveIntensity, ptr + offset, sizeof(float));
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
				m_hasDependencies = true;
			}
			else
			{
				return false;
			}
		}
	}

	if (!m_hasDependencies)
	{
		return false;
	}

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
			m_hasTerrainParams = true;
		}
	}

	if (0 != (materialFlags & kMaterialFlagTerrainSplat) && !m_hasTerrainParams)
	{
		return false;
	}

	return IsValid();
}

bool MaterialData::IsValid() const
{
	if (!m_hasProperties || !m_hasDependencies)
	{
		return false;
	}

	if (!IsKnownShadingModel(shadingModelId))
	{
		return false;
	}

	if (0 != (materialFlags & ~kKnownMaterialFlags))
	{
		return false;
	}

	if (!IsFiniteArray(albedo, 4) || !std::isfinite(roughness) || !std::isfinite(metallic) ||
		!IsFiniteArray(emissiveColor, 3) || !std::isfinite(emissiveIntensity))
	{
		return false;
	}

	if (4 < terrainLayerCount)
	{
		return false;
	}

	if (0 != (materialFlags & kMaterialFlagUseAlbedoMap) && !HasDependencySlot(dependencies, "ALBD"))
	{
		return false;
	}

	if (0 != (materialFlags & kMaterialFlagUseNormalMap) && !HasDependencySlot(dependencies, "NRML"))
	{
		return false;
	}

	if (0 != (materialFlags & kMaterialFlagUseOrmMap) && !HasDependencySlot(dependencies, "ORMS"))
	{
		return false;
	}

	if (0 != (materialFlags & kMaterialFlagEmissiveMap) && !HasDependencySlot(dependencies, "EMSV"))
	{
		return false;
	}

	if (0 != (materialFlags & kMaterialFlagTerrainSplat))
	{
		if (!m_hasTerrainParams || 0 == terrainLayerCount || !HasDependencySlot(dependencies, "SPL0"))
		{
			return false;
		}

		if (!IsFiniteArray(terrainSize, 2) || terrainSize[0] <= 0.0f || terrainSize[1] <= 0.0f ||
			!IsFiniteArray(&terrainLayerTileST[0][0], 4 * 4) ||
			!IsFiniteArray(&terrainLayerMetallicRoughness[0][0], 4 * 2))
		{
			return false;
		}
	}

	return true;
}
} // namespace EvAsset
