#pragma once
#include "AssetFormat.h"
#include "AssetFile.h"
#include <vector>

namespace EvAsset
{
struct MeshData : public AssetData
{
	Bounds				  boundsInfo;
	std::vector<Vertex>	  vertices;
	std::vector<uint32_t> indices;
	std::vector<SubMesh>  subMeshes;
	uint32_t			  indexFormat = 32;

	bool Deserialize(AssetFile& file) override;
	bool IsValid() const { return !vertices.empty() && !indices.empty(); }
};
} // namespace EvAsset
