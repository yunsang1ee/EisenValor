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

	struct Dependency
	{
		char slotType[4];
		Guid textureGuid;
	};
	std::vector<Dependency> dependencies;

	bool Deserialize(AssetFile& file) override;
	bool IsValid() const { return true; }
};
} // namespace EvAsset
