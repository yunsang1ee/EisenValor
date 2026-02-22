#include "AssetRegistryData.h"
#include "AssetFile.h"
#include <cstring>

bool EvAsset::AssetRegistryData::Deserialize(AssetFile& file)
{
	const ChunkEntry* listEntry = file.GetChunkEntry("LIST");
	if (nullptr != listEntry && 1 == listEntry->version)
	{
		size_t			 size = 0;
		const std::byte* ptr = static_cast<const std::byte*>(file.GetChunkDataPtr("LIST", size));

		constexpr size_t entryCountSize = 4;
		if (nullptr != ptr && entryCountSize <= size)
		{
			uint32_t count = 0;
			std::memcpy(&count, ptr, sizeof(uint32_t));

			entries.reserve(count);
			const std::byte* curr = ptr + entryCountSize;
			const std::byte* end = ptr + size;

			constexpr size_t guidSize = 16;
			constexpr size_t pathLenSize = 4;

			for (uint32_t i = 0; 0 < count && i < count; ++i)
			{
				if (curr + guidSize + pathLenSize > end)
				{
					break;
				}

				RegistryEntry entry;
				std::memcpy(&entry.guid, curr, guidSize);
				curr += guidSize;

				uint32_t pathLen = 0;
				pathLen = ReadUnaligned<uint32_t>(curr);
				curr += pathLenSize;

				if (curr + pathLen > end)
				{
					break;
				}

				entry.path.assign(reinterpret_cast<const char*>(curr), pathLen);
				curr += pathLen;

				entries.push_back(std::move(entry));
			}
		}
	}
	return true;
}
