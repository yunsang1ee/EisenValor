#pragma once
#include "AssetFormat.h"
#include "AssetFile.h"
#include <vector>

namespace EvAsset
{
struct SkinnedMeshData : public AssetData
{
	Bounds boundsInfo;

	std::vector<SkinnedVertex> vertices;
	std::vector<uint32_t>	   indices;
	std::vector<SubMesh>	   subMeshes;

	// Skin Mesh용
	std::vector<Bone>  bones;
	std::vector<float> offsetMatrices; // 4*4 행우선
	std::vector<Guid>  materialGuids;

	uint32_t indexFormat = 32;

	bool Deserialize(AssetFile& file) override;
	bool IsValid() const { return !vertices.empty() && !indices.empty() && !bones.empty(); }
};
} // namespace EvAsset
