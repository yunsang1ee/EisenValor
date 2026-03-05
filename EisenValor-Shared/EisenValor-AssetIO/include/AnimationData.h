#pragma once
#include "AssetFormat.h"
#include "AssetFile.h"
#include <vector>
#include <string>

namespace EvAsset
{
enum TrackFlags : uint32_t
{
	HasPos		 = 1 << 0,
	HasRot		 = 1 << 1,
	HasScale	 = 1 << 2,
	IsConstPos	 = 1 << 3,
	IsConstRot	 = 1 << 4,
	IsConstScale = 1 << 5
};

struct AnimationTrack
{
	uint64_t BoneNameHash;
	uint32_t Flags;

	std::vector<float> Positions; // If IsConstPos 3, else TotalFrames * 3
	std::vector<float> Rotations; // If IsConstRot 4, else TotalFrames * 4
	std::vector<float> Scales;	  // If IsConstScale 3, else TotalFrames * 3
};

struct AnimationData : public AssetData
{
	uint64_t nameHash = 0;
	float	 duration = 0.0f;
	float	 frameRate = 0.0f;
	uint32_t totalFrames = 0;
	Guid	 skeletonGuid{};

	std::vector<AnimationTrack> tracks;

	bool Deserialize(AssetFile& file) override;
};
} // namespace EvAsset
