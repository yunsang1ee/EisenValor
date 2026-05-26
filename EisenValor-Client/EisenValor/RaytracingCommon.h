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
#define MATERIAL_FLAG_TERRAIN_SPLAT (1 << 9)

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
	RAY_FLOAT4 emissive;
	RAY_FLOAT4 visibleEmissive;

	float	   roughness;
	float	   metallic;
	RAY_UINT   shadingModel;
	RAY_UINT   materialFlags;

	RAY_UINT albedoTextureIdx;
	RAY_UINT normalTextureIdx;
	RAY_UINT ormTextureIdx;
	RAY_UINT emissiveTextureIdx;

	RAY_UINT terrainSurfaceIdx;
	RAY_UINT pad0;
	RAY_UINT pad1;
	RAY_UINT pad2;
};

struct TerrainSurfaceGPUData
{
	RAY_UINT splatTextureIdx;
	RAY_UINT layerAlbedoTextureIdx0;
	RAY_UINT layerAlbedoTextureIdx1;
	RAY_UINT layerAlbedoTextureIdx2;

	RAY_UINT layerAlbedoTextureIdx3;
	RAY_UINT layerNormalTextureIdx0;
	RAY_UINT layerNormalTextureIdx1;
	RAY_UINT layerNormalTextureIdx2;

	RAY_UINT layerNormalTextureIdx3;
	RAY_UINT layerOrmTextureIdx0;
	RAY_UINT layerOrmTextureIdx1;
	RAY_UINT layerOrmTextureIdx2;

	RAY_UINT layerOrmTextureIdx3;
	RAY_UINT layerCount;
	RAY_UINT pad0;
	RAY_UINT pad1;

	RAY_FLOAT4 terrainSize;
	RAY_FLOAT4 layerTileST[4];
	RAY_FLOAT4 layerMetallicRoughness[4];
};

#define RESTIR_PRIMARY_HIT_VALID (1 << 0)
#define RESTIR_RESERVOIR_VALID (1 << 0)

#define RESTIR_SOURCE_CURRENT_PIXEL_FRESH_PATH 0
#define RESTIR_SHIFT_IDENTITY 0

struct RestirPrimaryHit
{
	RAY_FLOAT4 positionDistance;

	RAY_UINT packedNormal;
	RAY_UINT packedRoughness;
	RAY_UINT instanceId;
	RAY_UINT materialId;
	RAY_UINT geometryId;
	RAY_UINT flags;
};

struct RestirPathSample
{
	RAY_FLOAT4 contributionTarget; //.rgb = target contribution; .w = scalar target p_hat
	RAY_FLOAT4 throughputPdf; //.w = source path pdf used to derive contributionWeight
	RAY_FLOAT4 firstHitPositionDistance;
	RAY_FLOAT4 weightTerms; //.x = target, .y = contributionWeight, .z = misWeight, .w = shiftJacobian

	RAY_UINT packedFirstHitNormal;
	RAY_UINT packedFirstHitRoughness;
	RAY_UINT instanceId;
	RAY_UINT materialId;

	RAY_UINT geometryId;
	RAY_UINT pathLength;
	RAY_UINT sourceKind;
	RAY_UINT shiftKind;
};

struct RestirReservoir
{
	RestirPathSample sample;

	float	 resamplingWeightSum;
	float	 selectedResamplingWeight;
	RAY_UINT sampleCount;
	RAY_UINT flags;
};

#ifdef __cplusplus
static_assert(sizeof(RestirPrimaryHit) == 40);
static_assert(sizeof(RestirPathSample) == 96);
static_assert(sizeof(RestirReservoir) == 112);
#pragma pack(pop)
#else
// --- HLSL Only Helpers ---

struct RayPayload
{
	float3 color;
	uint   recursionDepth;

	float3 targetContribution;
	float  sourcePdf;

	float3 primaryHitPosition;
	float  primaryHitDistance;
	float3 primaryHitNormal;
	uint   primaryHitFlags;

	uint  instanceId;
	uint  materialId;
	uint  geometryId;
	float roughness;
};

RayPayload MakeDefaultRayPayload(uint recursionDepth)
{
	RayPayload payload;
	payload.color = 0.0f.xxx;
	payload.recursionDepth = recursionDepth;
	payload.targetContribution = 0.0f.xxx;
	payload.sourcePdf = 1.0f;
	payload.primaryHitPosition = 0.0f.xxx;
	payload.primaryHitDistance = -1.0f;
	payload.primaryHitNormal = float3(0.0f, 1.0f, 0.0f);
	payload.primaryHitFlags = 0u;
	payload.instanceId = 0xffffffffu;
	payload.materialId = 0xffffffffu;
	payload.geometryId = 0xffffffffu;
	payload.roughness = 1.0f;
	return payload;
}

static const float PI = 3.14159265359f;
static const float EPSILON = 0.0000001f;
static const float RAY_TMIN = 0.01f;
static const float RAY_TMAX = 10000.0f;
#endif

#endif // RAYTRACING_COMMON_H
