#include "AssetLoader.h"
#include "AssetMesh.h"
#include <fstream>

namespace AssetIO
{
	std::unique_ptr<AssetMeshData> AssetLoader::LoadStaticMesh(const std::wstring& filePath)
	{
		// TODO: Implement .evmesh parsing
		return nullptr;
	}
}
