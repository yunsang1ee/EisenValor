#pragma once
#include "AssetFormat.h"
#include "AssetFile.h"
#include <vector>
#include <cstring>

namespace EvAsset
{
struct TextureData : public AssetData
{
	bool				   isSRGB = false;
	TextureUsage		   usage = TextureUsage::Other;
	std::vector<std::byte> ddsBuffer;

	bool Deserialize(AssetFile& file) override;
	bool IsValid() const { return !ddsBuffer.empty(); }
};
} // namespace EvAsset
