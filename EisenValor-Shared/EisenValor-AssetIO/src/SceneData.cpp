#include "SceneData.h"
#include <cstring>
#include <print>

namespace EvAsset
{
bool SceneData::Deserialize(AssetFile& file)
{
	// 1. NODE Chunk
	size_t nodeSize = 0;
	const uint8_t* nodePtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("NODE", nodeSize));
	if (nodePtr && nodeSize >= 4)
	{
		uint32_t nodeCount = ReadUnaligned<uint32_t>(nodePtr);
		nodes.resize(nodeCount);
		std::memcpy(nodes.data(), nodePtr + 4, nodeCount * sizeof(Node));
		std::print("[SceneData] NODE Loaded: {} nodes", nodeCount);
	}

	// 2. COMP Chunk
	size_t compSize = 0;
	const uint8_t* dataPtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("COMP", compSize));
	if (dataPtr && compSize >= 4)
	{
		uint32_t entryCount = ReadUnaligned<uint32_t>(dataPtr);
		dataPtr += 4;

		components.resize(entryCount);

		for (uint32_t i = 0; i < entryCount; ++i)
		{
			ComponentEntry& entry = components[i];
			
			// Read Component Header
			entry.NodeIndex = ReadUnaligned<uint32_t>(dataPtr);
			dataPtr += 4;
			entry.TypeID = ReadUnaligned<uint64_t>(dataPtr);
			dataPtr += 8;
			entry.ComponentVersion = ReadUnaligned<uint32_t>(dataPtr);
			dataPtr += 4;
			entry.Size = ReadUnaligned<uint32_t>(dataPtr);
			dataPtr += 4;

			// Read Component Blob
			entry.Data.resize(entry.Size);
			std::memcpy(entry.Data.data(), dataPtr, entry.Size);
			dataPtr += entry.Size;
		}
		std::print("[SceneData] COMP Loaded: {} entries", entryCount);
	}

	return !nodes.empty();
}
} // namespace EvAsset
