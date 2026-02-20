#pragma once
#include "DxCommon.h"

struct DVertex
{
	DirectX::XMFLOAT3 position; // 3D 위치 (x, y, z)
	DirectX::XMFLOAT4 color;	// 색상 (r, g, b, a)
};

struct SkinnedVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 bitangent;
	
	// Skinning Data
	uint32_t blendIndices; // 4개의 uint8 인덱스
	float    blendWeights[4];
};
