#pragma once
#include <filesystem>
#include <concepts>
#include "AssetFormat.h"
#include "AssetFile.h"

namespace EvAsset
{
template <typename T>
concept IsAssetData = std::derived_from<T, AssetData>;

class AssetLoader
{
public:
	template <IsAssetData T>
	static bool Load(const std::filesystem::path& path, T& outData)
	{
		AssetFile file;
		if (!file.Load(path))
		{
			return false;
		}

		outData.assetGuid = file.GetHeader().assetGuid;
		outData.name = path.stem().string();

		return outData.Deserialize(file);
	}
};
} // namespace EvAsset
