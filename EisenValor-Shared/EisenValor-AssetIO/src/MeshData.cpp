#include "MeshData.h"
#include "AssetFile.h"
#include <cstring>
#include <print>

namespace EvAsset
{
bool MeshData::Deserialize(AssetFile& file)
{
	// 1. VERT (Vertex Data)
	const ChunkEntry* vertEntry = file.GetChunkEntry("VERT");
	if (vertEntry && 1 == vertEntry->version)
	{
		size_t		size = 0;
		const void* ptr = file.GetChunkDataPtr("VERT", size);
		if (nullptr != ptr)
		{
			vertices.resize(size / sizeof(Vertex));
			std::memcpy(vertices.data(), ptr, size);
		}
	}

	// 2. INDX (Index Data)
	const ChunkEntry* indxEntry = file.GetChunkEntry("INDX");
	if (indxEntry && 1 == indxEntry->version)
	{
		size_t			 size = 0;
		const std::byte* ptr = static_cast<const std::byte*>(file.GetChunkDataPtr("INDX", size));

		constexpr size_t indexHeaderSize = 8; // Index Header (8 Bytes) = Format(4) + Count(4)
		if (nullptr != ptr && indexHeaderSize <= size)
		{
			uint32_t format = ReadUnaligned<uint32_t>(ptr);
			uint32_t count = ReadUnaligned<uint32_t>(ptr + 4);

			size_t requiredDataSize = count * (16 == format ? 2 : 4);
			if (indexHeaderSize + requiredDataSize > size)
			{
				std::print(
					"[MeshData] INDX chunk size mismatch: expected {} bytes, got {}\n",
					indexHeaderSize + requiredDataSize, size
				);
				return false;
			}

			indexFormat = format;
			indices.reserve(count);

			if (32 == format)
			{
				indices.resize(count);
				std::memcpy(indices.data(), ptr + indexHeaderSize, count * 4);
			}
			else if (16 == format)
			{
				for (uint32_t i = 0; 0 < count && i < count; ++i)
				{
					uint16_t val;
					std::memcpy(&val, ptr + indexHeaderSize + i * (size_t)2, 2);
					indices.push_back(val);
				}
			}
		}
	}

	// 3. SUBM (SubMesh Data with Per-SubMesh Bounds)
	const ChunkEntry* submEntry = file.GetChunkEntry("SUBM");
	if (submEntry && 1 == submEntry->version)
	{
		size_t			 size = 0;
		const std::byte* ptr = static_cast<const std::byte*>(file.GetChunkDataPtr("SUBM", size));

		constexpr size_t subMeshHeaderSize = 4; // SubMesh Header (4 Bytes) = Count(4)
		if (nullptr != ptr && subMeshHeaderSize <= size)
		{
			uint32_t count = ReadUnaligned<uint32_t>(ptr);
			size_t	 requiredSize = subMeshHeaderSize + (count * sizeof(SubMesh));

			if (requiredSize <= size)
			{
				subMeshes.resize(count);
				std::memcpy(subMeshes.data(), ptr + subMeshHeaderSize, count * sizeof(SubMesh));
			}
			else
			{
				std::print("[MeshData] SUBM chunk size mismatch: expected {} bytes, got {}\n", requiredSize, size);
				return false;
			}
		}
	}

	// 4. BNDS (Global Mesh Bounds)
	const ChunkEntry* bndsEntry = file.GetChunkEntry("BNDS");
	if (bndsEntry && 1 == bndsEntry->version)
	{
		size_t		size = 0;
		const void* ptr = file.GetChunkDataPtr("BNDS", size);
		if (nullptr != ptr && sizeof(Bounds) <= size)
		{
			std::memcpy(&boundsInfo, ptr, sizeof(Bounds));
		}
	}

	return IsValid();
}
} // namespace EvAsset
