#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace AssetIO
{
	// ==========================================
	// Constants & Signatures
	// ==========================================
	inline constexpr uint32_t kSignatureMesh = 0x484D5645;	// "EVMH"
	inline constexpr uint32_t kAssetVersion = 1;

	// ==========================================
	// Vertex Structures (Packed)
	// ==========================================
#pragma pack(push, 1)

	struct AssetVertex
	{
		float px, py, pz;		// Position (12)
		float nx, ny, nz;		// Normal (12)
		float tx, ty, tz, tw;	// Tangent (16)
		float u, v;				// UV (8)
	};

#pragma pack(pop)

	// ==========================================
	// Mesh Data Containers
	// ==========================================

	struct AssetMeshData
	{
		uint32_t vertexCount;
		uint32_t indexCount;
		std::vector<AssetVertex> vertices;
		std::vector<uint32_t>    indices;
	};
}