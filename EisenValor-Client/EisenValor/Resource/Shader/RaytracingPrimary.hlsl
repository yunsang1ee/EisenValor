#define HLSL
#include "RaytracingCommon.h"

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);

cbuffer CameraConstants : register(b0, space0)
{
    float4x4 g_viewProjInverse;
};

StructuredBuffer<InstanceData> g_instanceBuffer : register(t1, space0);
StructuredBuffer<MaterialGPUData> g_materials : register(t2, space0);
StructuredBuffer<GeoInfo> g_geoTable : register(t3, space0);
StructuredBuffer<TerrainSurfaceGPUData> g_terrainSurfaces : register(t4, space0);

cbuffer TemporalAccumulationConstants : register(b2, space0)
{
    uint g_temporalFrameCount;
    uint g_temporalAccumulationEnabled;
    uint g_temporalAccumulationReset;
    uint g_emissionViewMode;
    uint g_environmentMode;
};

SamplerState g_sampler : register(s0, space0);

static const uint INVALID_TEXTURE_INDEX = 0xffffffffu;
#include "RaytracingMaterialEmission.hlsli"
#include "RaytracingTerrain.hlsli"
#include "RaytracingEnvironment.hlsli"
#include "RaytracingNormal.hlsli"

[shader("raygeneration")]
void RayGenMain()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;
    float2 ndc = (float2(pixelCoord) + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    float4 worldPos = mul(float4(ndc, 1.0f, 1.0f), g_viewProjInverse);
    worldPos /= worldPos.w;
    float4 cameraPos = mul(float4(0, 0, 0, 1), g_viewProjInverse);
    cameraPos /= cameraPos.w;
	
    RayDesc ray = { cameraPos.xyz, RAY_TMIN, normalize(worldPos.xyz - cameraPos.xyz), RAY_TMAX };
    RayPayload payload = { float3(0, 0, 0), 0 };
	
    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
	
    g_output[pixelCoord] = float4(max(0.0f.xxx, payload.color), 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    InstanceData inst = g_instanceBuffer[InstanceID()];
    GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + GeometryIndex()];
    MaterialGPUData mat = g_materials[geo.materialIdx];

    StructuredBuffer<Vertex> vBuffer = ResourceDescriptorHeap[inst.vertexBufferIdx];
    Buffer<uint> iBuffer = ResourceDescriptorHeap[inst.indexBufferIdx];

    uint triIdx = PrimitiveIndex();
    uint i0 = iBuffer[geo.indexBase + triIdx * 3 + 0];
    uint i1 = iBuffer[geo.indexBase + triIdx * 3 + 1];
    uint i2 = iBuffer[geo.indexBase + triIdx * 3 + 2];

    Vertex v0 = vBuffer[i0];
    Vertex v1 = vBuffer[i1];
    Vertex v2 = vBuffer[i2];
    float3 bary = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
    
    // Safety: Normal Transformation
    float3 normalObj = SafeNormalizeRay(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z, float3(0.0f, 1.0f, 0.0f));
    float3 worldNormal = SafeNormalizeRay(mul(normalObj, (float3x3) inst.worldInverse), float3(0.0f, 1.0f, 0.0f));
    float4 tangentPacked = v0.tangent * bary.x + v1.tangent * bary.y + v2.tangent * bary.z;
    float3 tangentCandidate = mul(tangentPacked.xyz, (float3x3) inst.worldMatrix);
    float3 tangent;
    float3 bitangent;
    BuildSafeTangentFrame(worldNormal, tangentCandidate, tangentPacked.w, tangent, bitangent);
    
    float2 uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;

    float3 albedo = mat.albedo.rgb;
    if (0 != (mat.materialFlags & MATERIAL_FLAG_TERRAIN_SPLAT))
    {
        TerrainSample terrainSample = SampleTerrainSplatPrimary(mat, uv, g_terrainSurfaces, g_sampler);
        albedo *= terrainSample.albedo;
        worldNormal = TangentNormalToWorld(terrainSample.normalTS, tangent, bitangent, worldNormal);
    }
    else if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ALBEDO_MAP))
    {
        Texture2D albedoTex = ResourceDescriptorHeap[mat.albedoTextureIdx];
        albedo *= albedoTex.SampleLevel(g_sampler, uv, 0).rgb;
    }
    if (dot(worldNormal, WorldRayDirection()) > 0.0f)
        worldNormal = -worldNormal;

    payload.color = EvaluateCameraVisibleMaterialEmission(mat, uv, g_emissionViewMode, g_sampler);
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    payload.color = SampleEnvironment(WorldRayDirection(), g_environmentMode);
}
