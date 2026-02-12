#pragma once
#include <cstdint>
#include <string_view>

/*
 - EisenValor Asset Pipeline Specification v2.1
 -
 - Signature: "EVMH" (Mesh), "EVTX" (Texture), "EVMT" (Material), "EVSK" (Skinned), "EVAN" (Anim), "EVSN" (Scene)
 - Version: 2
*/

namespace EvAsset
{
#pragma pack(push, 1)

// 128-bit GUID
struct Guid
{
	uint64_t low;
	uint64_t high;

	bool operator==(const Guid& other) const { return low == other.low && high == other.high; }
};

// NOTE: 파일 레이아웃 정의를 위한 구조체 (디스크 상의 포맷)
// 런타임 코드에서 reinterpret_cast로 접근 금지 (Udefined Behavior 발생 가능)
// memcpy 또는 필드별 파싱을 사용하여 정렬 안전성 유지
// 64 Bytes Global Header
struct AssetHeader
{
	char	 signature[4]; // "EVMH", "EVTX", etc.
	uint32_t version;	   // v2.1 = 2
	uint32_t headerSize;   // sizeof(AssetHeader) = 64
	uint32_t flags;		   // Reserved
	Guid	 assetGuid;	   // Unity GUID
	uint64_t fileSize;	   // 헤더를 포함한 전체 파일 크기
	uint32_t chunkCount;   // 청크 수
	uint8_t	 reserved[20]; // Padding to 64 bytes
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

struct MaterialProp
{
	uint64_t shaderNameHash; // FNV-1a 64-bit
	float	 albedo[4];
	float	 roughness;
	float	 metallic;
};

struct MaterialDepEntry
{
	char type[4]; // "ALBD", "NRML", "ORM ", "EMSV"
	Guid assetGuid;
};

#pragma pack(pop)

// FNV-1a Hash https://share.google/trFAqACv1zHhll7h8
constexpr uint64_t HashString(std::string_view str)
{
	uint64_t hash = 14695981039346656037ULL;
	for (char c : str)
	{
		hash ^= static_cast<uint8_t>(c);
		hash *= 1099511628211ULL;
	}
	return hash;
}
} // namespace EvAsset
