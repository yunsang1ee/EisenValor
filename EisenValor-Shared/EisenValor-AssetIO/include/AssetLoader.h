#pragma once

#include <string>
#include <memory>

namespace AssetIO
{
	class AssetMeshData;

	class AssetLoader
	{
	public:
		AssetLoader() = default;
		~AssetLoader() = default;

		static std::unique_ptr<AssetMeshData> LoadStaticMesh(const std::wstring& filePath);

	private:

	};
}
