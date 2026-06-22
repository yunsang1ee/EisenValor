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
	RAY_UINT stableGeometryId;
	RAY_UINT emissiveEntryIdx;
};

struct RestirEmissiveLightData
{
	RAY_UINT instanceIndex;
	RAY_UINT geometryIndex;
	RAY_UINT triangleCount;
	float	 selectionWeight;
};

struct InstanceData
{
	RAY_MATRIX worldMatrix;
	RAY_MATRIX worldInverse;

	RAY_UINT vertexBufferIdx;
	RAY_UINT indexBufferIdx;
	RAY_UINT geoInfoBaseIdx;
	RAY_UINT instanceID;
	RAY_UINT generation;
};

struct MaterialGPUData
{
	RAY_FLOAT4 albedo;
	RAY_FLOAT4 emissive;
	RAY_FLOAT4 visibleEmissive;

	float	 roughness;
	float	 metallic;
	RAY_UINT shadingModel;
	RAY_UINT materialFlags;

	RAY_UINT albedoTextureIdx;
	RAY_UINT normalTextureIdx;
	RAY_UINT ormTextureIdx;
	RAY_UINT emissiveTextureIdx;

	RAY_UINT terrainSurfaceIdx;
	RAY_UINT stableMaterialId;
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
#define RESTIR_PRIMARY_HIT_NEE_CANDIDATE (1u << 31)
#define RESTIR_PRIMARY_HIT_NEE_SUN (1u << 30)
#define RESTIR_PRIMARY_HIT_NEE_EMISSIVE (1u << 29)
#define RESTIR_RESERVOIR_VALID (1 << 0)

#define RESTIR_PATH_FLAG_NEE (1u << 0)
#define RESTIR_PATH_FLAG_NEE_SUN (1u << 1)
#define RESTIR_PATH_FLAG_NEE_EMISSIVE (1u << 2)
#define RESTIR_PATH_FLAG_EMISSIVE_HIT (1u << 3)
#define RESTIR_PATH_FLAG_SKY_ESCAPE (1u << 4)
#define RESTIR_PATH_FLAG_MIS_APPLIED (1u << 5)
#define RESTIR_PATH_EVENT_MASK ((1u << 6) - 1u)

#define RESTIR_BSDF_LOBE_DIFFUSE (1u << 0)
#define RESTIR_BSDF_LOBE_SPECULAR (1u << 1)
#define RESTIR_BSDF_LOBE_ALL (RESTIR_BSDF_LOBE_DIFFUSE | RESTIR_BSDF_LOBE_SPECULAR)

#define RESTIR_METADATA_LOBE_BEFORE_SHIFT 6u
#define RESTIR_METADATA_LOBE_AFTER_SHIFT 8u
#define RESTIR_METADATA_LOBE_MASK 0x3u
#define RESTIR_METADATA_DELTA_BEFORE_RC (1u << 10)
#define RESTIR_METADATA_DELTA_AFTER_RC (1u << 11)
#define RESTIR_METADATA_SHIFT_DATA_VALID (1u << 12)
#define RESTIR_METADATA_RC_FINAL (1u << 13)
#define RESTIR_METADATA_RC_NEE_TERMINAL (1u << 14)
#define RESTIR_METADATA_SOURCE_KIND_SHIFT 15u
#define RESTIR_METADATA_SOURCE_KIND_MASK (0x3u << RESTIR_METADATA_SOURCE_KIND_SHIFT)
#define RESTIR_METADATA_SHIFT_KIND_SHIFT 17u
#define RESTIR_METADATA_SHIFT_KIND_MASK (0x3u << RESTIR_METADATA_SHIFT_KIND_SHIFT)
#define RESTIR_METADATA_RC_SUFFIX_EMISSIVE_MIS (1u << 19)

#define RESTIR_SOURCE_CURRENT_PIXEL_FRESH_PATH 0
#define RESTIR_SOURCE_TEMPORAL_REUSE 1
#define RESTIR_SHIFT_IDENTITY 0
#define RESTIR_SHIFT_RECONNECTION 1

#define RESTIR_STYLIZED_NORMAL_STRENGTH 2.5f

struct RestirTemporalConstants
{
	RAY_UINT screenWidth;
	RAY_UINT screenHeight;
	RAY_UINT frameSeed;
	RAY_UINT historyValid;
	float	 normalThreshold;
	float	 positionThresholdScale;
	float	 temporalMCap;
	float	 jacobianRejectionThreshold;
	RAY_UINT instanceCount;
	RAY_UINT idToInstanceIndexCount;
	float	 shadingNormalStrength;
	RAY_UINT pad2;
	RAY_FLOAT3 cameraPosition;
	float	 pad3;
	RAY_FLOAT3 previousCameraPosition;
	float	 pad4;
};

struct RestirPrimaryHit
{
	RAY_FLOAT4 positionDistance;

	RAY_UINT packedNormal;
	RAY_UINT packedRoughnessFlags;
	RAY_UINT instanceId;
	RAY_UINT materialId;
	RAY_UINT geometryId;
	RAY_UINT geometryIndex;
	RAY_UINT primitiveIndex;
	RAY_UINT barycentrics;
};

struct RestirPathSample
{
	RAY_FLOAT4 contributionTarget; //.rgb = target contribution; .w = source cos(rc)/distance^2
	RAY_FLOAT4 throughputPdf;	   //.x = light pdf, .y = asfloat(metadata), .z/.w = source pdf before/after rc
	RAY_FLOAT4 weightTerms;		   //.x = target, .y = contributionWeight, .z = misWeight, .w = shiftJacobian

	RAY_UINT packedRcVertexWi;			 // oct16x2
	RAY_UINT packedRcVertexIrradianceXY; // half2
	RAY_UINT packedRcVertexIrradianceZ;	 // low half
	RAY_UINT reconnectInstanceGeneration;

	RAY_UINT reconnectInstanceId;
	RAY_UINT reconnectGeometryIndex;
	RAY_UINT reconnectPrimitiveIndex;
	RAY_UINT reconnectBarycentrics;
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
static_assert(sizeof(GeoInfo) == 20);
static_assert(sizeof(RestirEmissiveLightData) == 16);
static_assert(sizeof(RestirTemporalConstants) == 80);
static_assert(sizeof(RestirPrimaryHit) == 48);
static_assert(sizeof(RestirPathSample) == 80);
static_assert(sizeof(RestirReservoir) == 96);
#pragma pack(pop)
#else
// --- HLSL Only Helpers ---

static const float PI = 3.14159265359f;
static const float EPSILON = 0.0000001f;
static const float RAY_TMIN = 0.01f;
static const float RAY_TMAX = 10000.0f;
#endif

#endif // RAYTRACING_COMMON_H
