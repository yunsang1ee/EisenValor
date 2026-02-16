#pragma once
#include "AssetFormat.h"
#include <vector>
#include <string>

namespace EvAsset
{
struct AssetRegistryData : public AssetData
{
	struct RegistryEntry
	{
		Guid		guid;
		std::string path;
	};
	std::vector<RegistryEntry> entries;

	bool Deserialize(AssetFile& file) override;
};
} // namespace EvAsset
