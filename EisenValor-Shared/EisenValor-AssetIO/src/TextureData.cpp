#include "TextureData.h"
#include <cstring>

namespace EvAsset
{
bool TextureData::Deserialize(AssetFile& file)
{
	// 1. META (Texture Metadata)
	const ChunkEntry* metaEntry = file.GetChunkEntry("META");
	if (metaEntry && 1 == metaEntry->version)
	{
		size_t		size = 0;
		const void* ptr = file.GetChunkDataPtr("META", size);

		constexpr size_t minMetaSize = sizeof(TextureMeta);
		if (nullptr != ptr && minMetaSize <= size)
		{
			TextureMeta meta;
			std::memcpy(&meta, ptr, sizeof(TextureMeta));

			isSRGB = (0 != meta.isSRGB);
			usage = static_cast<TextureUsage>(meta.usage);
		}
	}

	// 2. DATA (Raw DDS Binary)
	const ChunkEntry* dataEntry = file.GetChunkEntry("DATA");
	if (dataEntry && 1 == dataEntry->version)
	{
		size_t		size = 0;
		const void* ptr = file.GetChunkDataPtr("DATA", size);

		if (nullptr != ptr && 0 < size)
		{
			ddsBuffer.resize(size);
			std::memcpy(ddsBuffer.data(), ptr, size);
		}
	}

	return IsValid();
}
} // namespace EvAsset