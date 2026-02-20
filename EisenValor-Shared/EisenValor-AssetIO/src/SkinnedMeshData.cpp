#include "SkinnedMeshData.h"
#include "AssetFile.h"
#include <cstring>
#include <print>

namespace EvAsset
{
bool SkinnedMeshData::Deserialize(AssetFile& file)
{
	// 1. 헤더 검증: 시그니처가 EVSK인지 확인
	const auto& header = file.GetHeader();
	if (std::strncmp(header.signature, "EVSK", 4) != 0)
	{
		std::string sig(header.signature, 4);
		std::print("[SkinnedMeshData] WRONG SIGNATURE: Expected EVSK, but got {}\n", sig);
		return false;
	}
	assetGuid = header.assetGuid;

	// 2. VERT 청크 파싱: SkinnedVertex 리스트
	size_t		vertSize = 0;
	const void* vertPtr = file.GetChunkDataPtr("VERT", vertSize);
	if (vertPtr)
	{
		size_t count = vertSize / sizeof(SkinnedVertex);
		vertices.resize(count);
		std::memcpy(vertices.data(), vertPtr, vertSize);
		std::print("[SkinnedMeshData] VERT Loaded: {} vertices\n", count);
	}

	// 3. INDX 청크 파싱: 16/32비트 포맷 대응
	size_t		   indxSize = 0;
	const uint8_t* indxPtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("INDX", indxSize));
	if (indxPtr && indxSize >= 8)
	{
		uint32_t	   format = ReadUnaligned<uint32_t>(indxPtr);
		uint32_t	   count = ReadUnaligned<uint32_t>(indxPtr + 4);
		const uint8_t* dataStart = indxPtr + 8;

		size_t requiredDataSize = count * (format == 16 ? 2 : 4);
		if (indxSize >= 8 + requiredDataSize)
		{
			indexFormat = 32;
			indices.reserve(count);

			if (format == 32)
			{
				indices.resize(count);
				std::memcpy(indices.data(), dataStart, count * 4);
			}
			else if (format == 16)
			{
				for (size_t i = 0; i < count; ++i)
				{
					uint16_t val;
					std::memcpy(&val, dataStart + i * 2, 2);
					indices.push_back(val);
				}
			}
			std::print("[SkinnedMeshData] INDX Loaded: {} indices\n", count);
		}
	}

	// 4. BONE 청크 파싱: 뼈대(Hierarchy,Rest Pose)
	size_t		   boneSize = 0;
	const uint8_t* bonePtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("BONE", boneSize));
	if (bonePtr && boneSize >= 4)
	{
		uint32_t count = ReadUnaligned<uint32_t>(bonePtr);
		bones.resize(count);
		std::memcpy(bones.data(), bonePtr + 4, count * sizeof(Bone));
		std::print("[SkinnedMeshData] BONE Loaded: {} bones\n", count);
	}

	// 5. OFFS 청크 파싱: Inverse Bind Poses
	size_t		offSize = 0;
	const void* offPtr = file.GetChunkDataPtr("OFFS", offSize);
	if (offPtr)
	{
		size_t floatCount = offSize / sizeof(float);
		offsetMatrices.resize(floatCount);
		std::memcpy(offsetMatrices.data(), offPtr, offSize);
		std::print("[SkinnedMeshData] OFFS Loaded: {} matrices\n", floatCount / 16);
	}

	if (!IsValid())
	{
		std::print(
			"[SkinnedMeshData] FINAL IsValid FAILED! (V:{}, I:{}, B:{})\n", (uint32_t)vertices.size(),
			(uint32_t)indices.size(), (uint32_t)bones.size()
		);
	}

	return IsValid();
}
} // namespace EvAsset
