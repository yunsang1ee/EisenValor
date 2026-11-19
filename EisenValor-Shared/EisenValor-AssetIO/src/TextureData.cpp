#include "TextureData.h"
#include <cstring>

namespace EvAsset
{
bool TextureData::Deserialize(AssetFile& file)
{
	const auto* dataEntry = file.GetChunkEntry("DATA");
	m_ddsSize = 0;
	m_dds = dataEntry && 1 == dataEntry->version ? file.ShareChunkData("DATA", m_ddsSize) : nullptr;

	isSRGB = false;
	usage = TextureUsage::Other;
	if (!IsValid())
	{
		return false;
	}

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

	return true;
}
} // namespace EvAsset
