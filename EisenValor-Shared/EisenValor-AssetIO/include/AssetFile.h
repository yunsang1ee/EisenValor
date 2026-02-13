#pragma once
#include "AssetFormat.h"
#include <unordered_map>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <span>

namespace EvAsset
{
class AssetFile
{
public:
	AssetFile() = default;
	~AssetFile();

	// 파일을 메모리에 매핑하고 헤더/청크 테이블을 파싱
	bool Load(const std::filesystem::path& path);
	void Unload();

	bool IsLoaded() const { return m_mappedView != nullptr; }

	const AssetHeader& GetHeader() const { return m_header; }
	const ChunkEntry*  GetChunkEntry(const char type[4]) const;
	const void*		   GetChunkDataPtr(const char type[4], size_t& outSize) const;

	template <typename T>
	std::span<const T> GetChunkSpan(const char type[4]) const
	{
		size_t		size = 0;
		const void* ptr = GetChunkDataPtr(type, size);
		if (!ptr)
		{
			return {};
		}

		return std::span<const T>(static_cast<const T*>(ptr), size / sizeof(T));
	}

private:
	HANDLE m_hFile = INVALID_HANDLE_VALUE;
	HANDLE m_hMapping = NULL;
	void*  m_mappedView = nullptr;
	size_t m_fileSize = 0;

	AssetHeader				m_header{};
	std::vector<ChunkEntry> m_chunkTable;

	std::unordered_map<uint32_t, size_t> m_chunkMap;
};
} // namespace EvAsset
