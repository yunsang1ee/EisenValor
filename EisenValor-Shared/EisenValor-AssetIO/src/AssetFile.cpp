#include "AssetFile.h"
#include <cstring>
#include <print>

namespace EvAsset
{
static uint32_t MakeTypeKey(const char t[4])
{
	uint32_t k = 0;
	std::memcpy(&k, t, 4);
	return k;
}

AssetFile::~AssetFile()
{
	Unload();
}

bool AssetFile::Load(const std::filesystem::path& path)
{
	Unload();

	m_hFile = ::CreateFileW(
		path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr
	);

	if (INVALID_HANDLE_VALUE == m_hFile)
	{
		std::print("[AssetFile] Failed to open file: {}\n", path.string());
		return false;
	}

	LARGE_INTEGER fileSize;
	if (!::GetFileSizeEx(m_hFile, &fileSize))
	{
		std::print("[AssetFile] Failed to get file size: {}\n", path.string());
		Unload();
		return false;
	}
	m_fileSize = static_cast<size_t>(fileSize.QuadPart);

	if (sizeof(AssetHeader) > m_fileSize)
	{
		std::print("[AssetFile] File too small: {}\n", path.string());
		Unload();
		return false;
	}

	m_hMapping = ::CreateFileMappingW(m_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!m_hMapping)
	{
		std::print("[AssetFile] Failed to create file mapping: {}\n", path.string());
		Unload();
		return false;
	}

	m_mappedView = ::MapViewOfFile(m_hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!m_mappedView)
	{
		std::print("[AssetFile] Failed to map view of file: {}\n", path.string());
		Unload();
		return false;
	}

	std::memcpy(&m_header, m_mappedView, sizeof(AssetHeader));

	if (2 > m_header.version)
	{
		std::print("[AssetFile] Version mismatch: {} (Path: {})\n", m_header.version, path.string());
		Unload();
		return false;
	}

	if (64 > m_header.headerSize || m_header.headerSize > m_fileSize)
	{
		std::print("[AssetFile] Invalid header size: {} (Path: {})\n", m_header.headerSize, path.string());
		Unload();
		return false;
	}

	if (m_header.fileSize != m_fileSize)
	{
		std::print("[AssetFile] File size mismatch (Path: {})\n", path.string());
		Unload();
		return false;
	}

	if (0 < m_header.chunkCount)
	{
		size_t tableSize = static_cast<size_t>(m_header.chunkCount) * sizeof(ChunkEntry);
		if (m_header.headerSize + tableSize > m_fileSize)
		{
			Unload();
			return false;
		}

		const std::byte* viewBase = static_cast<const std::byte*>(m_mappedView);

		m_chunkTable.resize(m_header.chunkCount);
		std::memcpy(m_chunkTable.data(), viewBase + m_header.headerSize, tableSize);

		for (size_t i = 0; i < m_chunkTable.size(); ++i)
		{
			uint32_t typeKey = MakeTypeKey(m_chunkTable[i].type);
			m_chunkMap[typeKey] = i;
		}
	}

	return true;
}

void AssetFile::Unload()
{
	if (m_mappedView)
	{
		::UnmapViewOfFile(m_mappedView);
		m_mappedView = nullptr;
	}

	if (m_hMapping)
	{
		::CloseHandle(m_hMapping);
		m_hMapping = NULL;
	}

	if (INVALID_HANDLE_VALUE != m_hFile)
	{
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	std::memset(&m_header, 0, sizeof(AssetHeader));
	m_chunkTable.clear();
	m_chunkMap.clear();
	m_fileSize = 0;
}

const ChunkEntry* AssetFile::GetChunkEntry(const char type[4]) const
{
	if (m_chunkMap.empty())
		return nullptr;

	uint32_t typeKey = MakeTypeKey(type);
	auto	 it = m_chunkMap.find(typeKey);
	if (m_chunkMap.end() != it)
	{
		return &m_chunkTable[it->second];
	}
	return nullptr;
}

const void* AssetFile::GetChunkDataPtr(const char type[4], size_t& outSize) const
{
	const ChunkEntry* entry = GetChunkEntry(type);
	if (!entry)
	{
		outSize = 0;
		return nullptr;
	}

	if (entry->offset + entry->size > m_fileSize)
	{
		outSize = 0;
		return nullptr;
	}

	outSize = static_cast<size_t>(entry->size);
	return static_cast<const std::byte*>(m_mappedView) + entry->offset;
}
} // namespace EvAsset
