#include "AnimationData.h"
#include <cstring>

namespace EvAsset
{
bool AnimationData::Deserialize(AssetFile& file)
{
	// 1. AMET
	const ChunkEntry* ametEntry = file.GetChunkEntry("AMET");
	if (ametEntry && 1 == ametEntry->version)
	{
		size_t		size = 0;
		const void* ptr = file.GetChunkDataPtr("AMET", size);
		if (ptr && size >= (8 + 4 + 4 + 4 + 16))
		{
			const uint8_t* bytePtr = static_cast<const uint8_t*>(ptr);
			std::memcpy(&nameHash, bytePtr, 8);
			bytePtr += 8;
			std::memcpy(&duration, bytePtr, 4);
			bytePtr += 4;
			std::memcpy(&frameRate, bytePtr, 4);
			bytePtr += 4;
			std::memcpy(&totalFrames, bytePtr, 4);
			bytePtr += 4;
			std::memcpy(&skeletonGuid, bytePtr, 16);
		}
	}

	// 2. ATRK
	const ChunkEntry* atrkEntry = file.GetChunkEntry("ATRK");
	if (atrkEntry && 1 == atrkEntry->version)
	{
		size_t		size = 0;
		const void* ptr = file.GetChunkDataPtr("ATRK", size);
		if (ptr && size >= 4)
		{
			const uint8_t* bytePtr = static_cast<const uint8_t*>(ptr);
			uint32_t	   trackCount = 0;
			std::memcpy(&trackCount, bytePtr, 4);
			bytePtr += 4;

			tracks.reserve(trackCount);
			for (uint32_t i = 0; i < trackCount; ++i)
			{
				AnimationTrack track;
				// BoneIndex 2바이트
				std::memcpy(&track.BoneIndex, bytePtr, 2);
				bytePtr += 2;
				// Flags 4바이트
				std::memcpy(&track.Flags, bytePtr, 4);
				bytePtr += 4;

				// Position (float3 * N)
				if (track.Flags & HasPos)
				{
					uint32_t count = (track.Flags & IsConstPos) ? 1 : totalFrames;
					track.Positions.resize(count * 3);
					std::memcpy(track.Positions.data(), bytePtr, count * 12);
					bytePtr += count * 12;
				}

				// Rotation (float4 * N)
				if (track.Flags & HasRot)
				{
					uint32_t count = (track.Flags & IsConstRot) ? 1 : totalFrames;
					track.Rotations.resize(count * 4);
					std::memcpy(track.Rotations.data(), bytePtr, count * 16);
					bytePtr += count * 16;
				}

				// Scale (float3 * N)
				if (track.Flags & HasScale)
				{
					uint32_t count = (track.Flags & IsConstScale) ? 1 : totalFrames;
					track.Scales.resize(count * 3);
					std::memcpy(track.Scales.data(), bytePtr, count * 12);
					bytePtr += count * 12;
				}

				tracks.push_back(std::move(track));
			}
		}
	}

	return !tracks.empty();
}
} // namespace EvAsset
