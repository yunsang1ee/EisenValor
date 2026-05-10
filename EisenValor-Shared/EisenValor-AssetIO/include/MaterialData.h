#pragma once
#include "AssetFormat.h"
#include "AssetFile.h"
#include <vector>
#include <cstring>

namespace EvAsset
{
struct MaterialData : public AssetData
{
	ShadingModel shadingModelId = ShadingModel::LitPbr;
	uint32_t	 materialFlags = 0;
	float		 albedo[4]{1.0f, 1.0f, 1.0f, 1.0f};
	float		 roughness = 1.0f;
	float		 metallic = 0.0f;
	float		 emissiveColor[3]{0.0f, 0.0f, 0.0f};
	float		 emissiveIntensity = 0.0f;
	uint32_t	 terrainLayerCount = 0;
	float		 terrainSize[2]{0.0f, 0.0f};
	float		 terrainLayerTileST[4][4]{};
	float		 terrainLayerMetallicRoughness[4][2]{};

	struct Dependency
	{
		char slotType[4];
		Guid textureGuid;
	};
	std::vector<Dependency> dependencies;

	bool Deserialize(AssetFile& file) override;
	bool IsValid() const;

private:
	bool m_hasProperties = false;
	bool m_hasDependencies = false;
	bool m_hasTerrainParams = false;
};
} // namespace EvAsset
