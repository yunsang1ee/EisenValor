// =======================================================================================
// RaytracingCommon.h - Shared between C++ and HLSL
// =======================================================================================

#ifndef RAYTRACING_COMMON_H
#define RAYTRACING_COMMON_H

#ifdef __cplusplus
#include <DirectXMath.h>
#include <cstdint>
#define RAY_FLOAT4 DirectX::XMFLOAT4
#define RAY_FLOAT3 DirectX::XMFLOAT3
#define RAY_FLOAT2 DirectX::XMFLOAT2
#define RAY_MATRIX DirectX::XMFLOAT4X4
#define RAY_UINT uint32_t

#else
#define RAY_FLOAT4 float4
#define RAY_FLOAT3 float3
#define RAY_FLOAT2 float2
#define RAY_MATRIX float4x4
#define RAY_UINT uint
#endif

// --- Material Flags ---
#define MATERIAL_FLAG_NONE 0
#define MATERIAL_FLAG_USE_ALBEDO_MAP (1 << 0)
#define MATERIAL_FLAG_USE_NORMAL_MAP (1 << 1)
#define MATERIAL_FLAG_USE_ORM_MAP (1 << 2)
#define MATERIAL_FLAG_UNITY_PACKING (1 << 3)
#define MATERIAL_FLAG_ALPHA_TEST (1 << 4)
#define MATERIAL_FLAG_DOUBLE_SIDED (1 << 5)
#define MATERIAL_FLAG_EMISSIVE_MAP (1 << 6)
#define MATERIAL_FLAG_TRANSPARENT (1 << 7)
#define MATERIAL_FLAG_IGNORE_LIGHTING (1 << 8)

#ifdef __cplusplus
#pragma pack(push, 1)
#endif

#ifndef __cplusplus
struct Vertex
{
	float3 position;
	float3 normal;
	float4 tangent;
	float2 uv;
};
#endif

struct SkinnedVertex
{
	RAY_FLOAT3 position;
	RAY_FLOAT3 normal;
	RAY_FLOAT4 tangent;
	RAY_FLOAT2 uv;
	RAY_UINT   blendIndices;
	RAY_FLOAT4 blendWeights;
};

struct GeoInfo
{
	RAY_UINT vertexBase;
	RAY_UINT indexBase;
	RAY_UINT materialIdx;
	RAY_UINT pad0;
};

struct InstanceData
{
	RAY_MATRIX worldMatrix;
	RAY_MATRIX worldInverse;

	RAY_UINT vertexBufferIdx;
	RAY_UINT indexBufferIdx;
	RAY_UINT geoInfoBaseIdx;
	RAY_UINT instanceID;
};

struct MaterialGPUData
{
	RAY_FLOAT4 albedo;
	float	   roughness;
	float	   metallic;
	RAY_UINT   shadingModel;
	RAY_UINT   materialFlags;

	RAY_UINT albedoTextureIdx;
	RAY_UINT normalTextureIdx;
	RAY_UINT ormTextureIdx;
	RAY_UINT emissiveTextureIdx;
};

#ifdef __cplusplus
#pragma pack(pop)
#else
// --- HLSL Only Helpers ---
struct RayPayload
{
	float3 color;
	uint   recursionDepth;
};

static const float PI = 3.14159265359f;
static const float EPSILON = 0.0000001f;
static const float RAY_TMIN = 0.01f;
static const float RAY_TMAX = 10000.0f;
#endif

#endif // RAYTRACING_COMMON_H
