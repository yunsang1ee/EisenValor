#pragma once
#include "AssetFormat.h"
#include <vector>
#include <string>

namespace EvAsset
{
struct MeshData
{
	std::string name;
	Guid		assetGuid;
	Bounds		boundsInfo;

	std::vector<Vertex>	  vertices;
	std::vector<uint32_t> indices;
	std::vector<SubMesh>  subMeshes;

	uint32_t indexFormat = 32;

	bool IsValid() const { return !vertices.empty() && !indices.empty(); }
};
} // namespace EvAsset
