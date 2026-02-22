// =======================================================================================
// Raytracing Common Definitions (v2.1)
// =======================================================================================

#ifndef RAYTRACING_COMMON_HLSLI
#define RAYTRACING_COMMON_HLSLI

// --- Material Flags ---
#define MATERIAL_FLAG_NONE              0
#define MATERIAL_FLAG_USE_ALBEDO_MAP    (1 << 0)
#define MATERIAL_FLAG_USE_NORMAL_MAP    (1 << 1)
#define MATERIAL_FLAG_USE_ORM_MAP       (1 << 2)
#define MATERIAL_FLAG_UNITY_PACKING     (1 << 3)
#define MATERIAL_FLAG_DOUBLE_SIDED      (1 << 4)

// --- Data Structures ---

struct Vertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

struct GeoInfo
{
    uint vertexBase;
    uint indexBase;
    uint materialIdx;
    uint pad0;
};

struct InstanceData
{
    float4x4 worldMatrix;
    float4x4 worldIT;
    
    uint vertexBufferIdx;
    uint indexBufferIdx;
    uint geoInfoBaseIdx;
    uint instanceID;
};

struct MaterialGPUData
{
    float4 albedo;
    float roughness;
    float metallic;
    uint shadingModel;
    uint materialFlags;

    uint albedoTextureIdx;
    uint normalTextureIdx;
    uint ormTextureIdx;
    uint pad0;
};

struct RayPayload
{
    float3 color;
    uint recursionDepth;
};

static const float PI = 3.14159265359f;
static const float EPSILON = 0.0000001f;
static const float RAY_TMIN = 0.001f;
static const float RAY_TMAX = 10000.0f;

#endif // RAYTRACING_COMMON_HLSLI
