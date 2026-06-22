#pragma once
#include <cstdint>
#include <string_view>
#include <string>
#include <format>
#include <bit>   
#include <array> 
#include <cstddef>
#include <cstring>

/*
 - EisenValor Asset Pipeline Specification v2.1
 -
 - Signature: "EVMH" (Mesh), "EVTX" (Texture), "EVMT" (Material), "EVSK" (Skinned), "EVAN" (Anim), "EVSN" (Scene)
 - Version: 2
*/

namespace EvAsset
{
#pragma pack(push, 1)

struct Guid
{
	uint8_t data[16];

	constexpr bool IsNull() const
	{
		for (uint8_t byte : data)
		{
			if (byte != 0)
			{
				return false;
			}
		}
		return true;
	}

	bool operator==(const Guid& other) const 
	{ 
		return 0 == std::memcmp(data, other.data, 16); 
	}
};

// GUID를 읽기 위한 Format
#pragma pack(pop)
} // namespace EvAsset

namespace EvAsset
{
#pragma pack(push, 1)

struct GuidHash
{
	size_t operator()(const EvAsset::Guid& guid) const
	{
		uint64_t low, high;
		std::memcpy(&low, guid.data, 8);
		std::memcpy(&high, guid.data + 8, 8);
		return std::hash<uint64_t>{}(low) ^ (std::hash<uint64_t>{}(high) << 1);
	}
};

// NOTE: 파일 레이아웃 정의를 위한 구조체 (디스크 상의 포맷)
struct AssetHeader
{
	char	  signature[4]; // "EVMH", "EVTX", etc.
	uint32_t  version;		// v2.1 = 2
	uint32_t  headerSize;	// sizeof(AssetHeader) = 64
	uint32_t  flags;		// Reserved
	Guid	  assetGuid;	// Unity GUID
	uint64_t  fileSize;		// 헤더를 포함한 전체 파일 크기
	uint32_t  chunkCount;	// 청크 수
	std::byte reserved[20];
};

// Chunk Entry in Chunk Table
struct ChunkEntry
{
	char	 type[4];		   // "VERT", "INDX", "SUBM", "BNDS", "META", "DATA", "PROP", "DEPS", etc.
	uint32_t version;		   // 청크 버전
	uint64_t offset;		   // 파일 시작부터의 오프셋 (0)
	uint64_t size;			   // 디스크 상의 원시 페이로드 크기
	uint64_t uncompressedSize; // 디코딩된 크기 (압축되지 않은 경우 Size == UncompressedSize)
};

// --- Static Mesh Payloads ---

struct Vertex
{
	float position[3];
	float normal[3];
	float tangent[4]; // w: bitangent sign
	float uv0[2];
};

struct SkinnedVertex
{
	Vertex  staticVertex;
	uint8_t blendIndices[4];
	float   blendWeights[4];
};

struct Bone
{
	uint64_t nameHash;
	int32_t  parentIndex;
	float	 restPos[3];
	float	 restRot[4]; // Quaternion
	float	 restScale[3];
};

struct SubMesh
{
	uint32_t indexOffset; // 인덱스 배열에서 시작 인덱스
	uint32_t indexCount;
	uint32_t materialSlot;

	float aabbmin[3];
	float aabbmax[3];
};

struct ChunkIndexMeta
{
	uint32_t indexFormat; // 16 or 32
	uint32_t indexCount;
};

struct Bounds
{
	float aabbmin[3];
	float aabbmax[3];
	float sphereCenter[3];
	float sphereRadius;
};

// --- Texture Payloads ---

enum class TextureUsage : uint32_t
{
	Albedo = 0,
	Normal = 1,
	ORM = 2,
	Emissive = 3,
	UI = 4,
	Other = 5
};

struct TextureMeta
{
	uint32_t isSRGB;
	uint32_t usage; // TextureUsage
};

// --- Material Payloads ---

enum class ShadingModel : uint32_t
{
	LitPbr = 0,
	Unlit = 1,
	ClearCoat = 2,
	Skin = 3,
	Custom = 99
};

struct MaterialProp
{
	ShadingModel shadingModelId;
	uint32_t	 materialFlags;
	float		 albedo[4];
	float		 roughness;
	float		 metallic;
};

struct MaterialDepEntry
{
	char type[4]; // "ALBD", "NRML", "ORM ", "EMSV", "SPL0", "LA0A".."LA3A"
	Guid assetGuid;
};

#pragma pack(pop)

struct AssetData
{
	Guid		assetGuid;
	std::string name;

	virtual ~AssetData() = default;
	virtual bool Deserialize(class AssetFile& file) = 0;
};

// FNV-1a Hash https://share.google/trFAqACv1zHhll7h8
constexpr uint64_t HashString(std::string_view str)
{
	uint64_t hash = 14695981039346656037ULL;
	for (char c : str)
	{
		hash ^= static_cast<uint64_t>(static_cast<std::byte>(c));
		hash *= 1099511628211ULL;
	}
	return hash;
}

template <typename T>
static T ReadUnaligned(const void* ptr)
{
	T val;
	std::memcpy(&val, ptr, sizeof(T));
	return val;
}
} // namespace EvAsset

template <>
struct std::formatter<EvAsset::Guid> : std::formatter<std::string>
{
	auto format(const EvAsset::Guid& guid, std::format_context& ctx) const
	{
		std::string result;
		result.reserve(32);
		for (uint8_t byte : guid.data)
		{
			result += std::format("{:02X}", byte);
		}
		return std::formatter<std::string>::format(result, ctx);
	}
};
