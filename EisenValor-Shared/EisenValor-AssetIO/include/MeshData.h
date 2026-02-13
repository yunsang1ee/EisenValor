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

struct SkinnedMeshData
{
	std::string name;
	Guid		assetGuid;
	Bounds		boundsInfo;

	std::vector<SkinnedVertex> vertices;
	std::vector<uint32_t>	   indices;
	std::vector<SubMesh>	   subMeshes;
	
	// Skin Mesh용
	std::vector<Bone>		   bones;
	std::vector<float>		   offsetMatrices; // 4*4 행우선

	uint32_t indexFormat = 32;

	bool IsValid() const { return !vertices.empty() && !indices.empty() && !bones.empty(); }
};
} // namespace EvAsset
