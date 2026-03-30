#include "SceneData.h"
#include <cstring>
#include <print>

namespace EvAsset
{
bool SceneData::Deserialize(AssetFile& file)
{
	// 1. NODE Chunk
	size_t		   nodeSize = 0;
	const uint8_t* nodePtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("NODE", nodeSize));
	if (nodePtr && nodeSize >= 4)
	{
		uint32_t	 nodeCount = ReadUnaligned<uint32_t>(nodePtr);
		const size_t requiredSize = 4 + static_cast<size_t>(nodeCount) * sizeof(Node);
		if (nodeSize != requiredSize)
		{
			std::print("[SceneData] NODE size mismatch: expected {}, got {}\n", requiredSize, nodeSize);
			return false;
		}

		nodes.resize(nodeCount);
		if (nodeCount > 0)
		{
			std::memcpy(nodes.data(), nodePtr + 4, nodeCount * sizeof(Node));
		}
		std::print("[SceneData] NODE Loaded: {} nodes", nodeCount);
	}
	else
	{
		std::print("[SceneData] NODE chunk missing or too small\n");
		return false;
	}

	// 2. COMP Chunk
	size_t		   compSize = 0;
	const uint8_t* dataPtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("COMP", compSize));
	if (dataPtr && compSize >= 4)
	{
		uint32_t entryCount = ReadUnaligned<uint32_t>(dataPtr);
		dataPtr += 4;
		size_t remainingSize = compSize - 4;

		components.resize(entryCount);

		for (uint32_t i = 0; i < entryCount; ++i)
		{
			ComponentEntry&	 entry = components[i];
			constexpr size_t kComponentHeaderSize =
				sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint32_t);

			if (remainingSize < kComponentHeaderSize)
			{
				std::print("[SceneData] COMP header truncated at entry {}\n", i);
				return false;
			}

			// Read Component Header
			entry.NodeIndex = ReadUnaligned<uint32_t>(dataPtr);
			dataPtr += 4;
			entry.TypeID = ReadUnaligned<uint64_t>(dataPtr);
			dataPtr += 8;
			entry.ComponentVersion = ReadUnaligned<uint32_t>(dataPtr);
			dataPtr += 4;
			entry.Size = ReadUnaligned<uint32_t>(dataPtr);
			dataPtr += 4;
			remainingSize -= kComponentHeaderSize;

			if (entry.NodeIndex >= nodes.size())
			{
				std::print("[SceneData] COMP node index out of range: {} (nodes={})\n", entry.NodeIndex, nodes.size());
				return false;
			}

			if (remainingSize < entry.Size)
			{
				std::print("[SceneData] COMP payload truncated at entry {}\n", i);
				return false;
			}

			// Read Component Blob
			entry.Data.resize(entry.Size);
			if (entry.Size > 0)
			{
				std::memcpy(entry.Data.data(), dataPtr, entry.Size);
			}
			dataPtr += entry.Size;
			remainingSize -= entry.Size;
		}

		if (remainingSize != 0)
		{
			std::print("[SceneData] COMP trailing bytes detected: {}\n", remainingSize);
			return false;
		}
		std::print("[SceneData] COMP Loaded: {} entries", entryCount);
	}

	return !nodes.empty();
}
} // namespace EvAsset
