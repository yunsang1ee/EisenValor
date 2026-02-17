#pragma once
#include "MeshData.h"
#include "SkinnedMeshData.h"
#include <string>
#include <filesystem>

namespace EvAsset
{
class MeshLoader
{
public:
	// 파일을 읽어 MeshData 구조체를 채워 반환합니다.
	static bool LoadMesh(const std::filesystem::path& path, MeshData& outData);

	// .obj 파일을 읽어 MeshData 구조체를 채워 반환합니다.
	static bool LoadMeshFromObj(const std::filesystem::path& path, MeshData& outData);
	};
	} // namespace EvAsset
