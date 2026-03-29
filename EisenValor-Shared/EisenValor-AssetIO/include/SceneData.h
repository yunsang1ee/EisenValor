#pragma once
#include "AssetFormat.h"
#include "AssetFile.h"
#include <vector>
#include <string>

namespace EvAsset
{
// Node
struct Node
{
	int32_t ParentIndex; // root=-1
	float	LocalPos[3];
	float	LocalRot[4]; // quaternion
	float	LocalScale[3];
};

struct SceneData : public AssetData
{
	// ComponentEntry (Header + Data)
	struct ComponentEntry
	{
		uint32_t			   NodeIndex; // 어느 노드에 붙는지
		uint64_t			   TypeID;	  // FNV-1a 64-bit
		uint32_t			   ComponentVersion;
		uint32_t			   Size;
		std::vector<std::byte> Data; // 데이터
	};

	std::vector<Node>			nodes;
	std::vector<ComponentEntry> components;

	bool Deserialize(AssetFile& file) override;
};
} // namespace EvAsset
