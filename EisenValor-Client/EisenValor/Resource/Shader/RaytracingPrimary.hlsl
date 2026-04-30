#define HLSL
#include "RaytracingCommon.h"

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4>             g_output : register(u0, space0);

cbuffer CameraConstants : register(b0, space0)
{
    float4x4 g_viewProjInverse;
};

StructuredBuffer<InstanceData>    g_instanceBuffer : register(t1, space0);
StructuredBuffer<MaterialGPUData> g_materials      : register(t2, space0);
StructuredBuffer<GeoInfo>         g_geoTable       : register(t3, space0);
StructuredBuffer<TerrainSurfaceGPUData> g_terrainSurfaces : register(t4, space0);

SamplerState g_sampler : register(s0, space0);

static const uint INVALID_TEXTURE_INDEX = 0xffffffffu;

struct TerrainSample
{
    float3 albedo;
    float3 normalTS;
};

void AccumulateTerrainLayer(
    inout TerrainSample blended,
    inout float weightSum,
    uint albedoTextureIdx,
    uint normalTextureIdx,
    float weight,
    float2 terrainXZ,
    float4 tileST)
{
    if (weight <= 0.0f)
    {
        return;
    }

    float2 layerUV = terrainXZ * tileST.xy + tileST.zw;
    float3 layerAlbedo = 1.0f.xxx;
    if (albedoTextureIdx != INVALID_TEXTURE_INDEX)
    {
        Texture2D albedoTexture = ResourceDescriptorHeap[albedoTextureIdx];
        layerAlbedo = albedoTexture.SampleLevel(g_sampler, layerUV, 0).rgb;
    }

    float3 layerNormalTS = float3(0.0f, 0.0f, 1.0f);
    if (normalTextureIdx != INVALID_TEXTURE_INDEX)
    {
        Texture2D normalTexture = ResourceDescriptorHeap[normalTextureIdx];
        float2 encodedXY = normalTexture.SampleLevel(g_sampler, layerUV, 0).xy * 2.0f - 1.0f;
        layerNormalTS = float3(encodedXY, sqrt(saturate(1.0f - dot(encodedXY, encodedXY))));
    }

    blended.albedo += layerAlbedo * weight;
    blended.normalTS += layerNormalTS * weight;
    weightSum += weight;
}

float4 SampleTerrainSplatWeights(Texture2D splatTexture, float2 terrainUV)
{
    uint width = 0;
    uint height = 0;
    splatTexture.GetDimensions(width, height);
    if (width == 0 || height == 0)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float2 texel = saturate(terrainUV) * float2(width - 1, height - 1);
    uint2 p0 = (uint2)floor(texel);
    uint2 p1 = min(p0 + 1, uint2(width - 1, height - 1));
    float2 t = texel - float2(p0);
    int2 ip0 = int2(p0);
    int2 ip1 = int2(p1);

    float4 w00 = splatTexture.Load(int3(ip0, 0));
    float4 w10 = splatTexture.Load(int3(ip1.x, ip0.y, 0));
    float4 w01 = splatTexture.Load(int3(ip0.x, ip1.y, 0));
    float4 w11 = splatTexture.Load(int3(ip1, 0));
    return lerp(lerp(w00, w10, t.x), lerp(w01, w11, t.x), t.y);
}

TerrainSample SampleTerrainSplat(MaterialGPUData mat, float2 terrainUV)
{
    TerrainSample result;
    result.albedo = 1.0f.xxx;
    result.normalTS = float3(0.0f, 0.0f, 1.0f);

    if (mat.terrainSurfaceIdx == INVALID_TEXTURE_INDEX)
    {
        return result;
    }

    TerrainSurfaceGPUData terrain = g_terrainSurfaces[mat.terrainSurfaceIdx];
    if (terrain.splatTextureIdx == INVALID_TEXTURE_INDEX)
    {
        return result;
    }

    Texture2D splatTexture = ResourceDescriptorHeap[terrain.splatTextureIdx];
    float4 weights = SampleTerrainSplatWeights(splatTexture, terrainUV);
    float2 terrainXZ = terrainUV * terrain.terrainSize.xy;
    uint layerCount = terrain.layerCount;

    TerrainSample blended;
    blended.albedo = 0.0f.xxx;
    blended.normalTS = 0.0f.xxx;
    float weightSum = 0.0f;
    if (0 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx0, terrain.layerNormalTextureIdx0, weights.r, terrainXZ, terrain.layerTileST[0]);
    }
    if (1 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx1, terrain.layerNormalTextureIdx1, weights.g, terrainXZ, terrain.layerTileST[1]);
    }
    if (2 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx2, terrain.layerNormalTextureIdx2, weights.b, terrainXZ, terrain.layerTileST[2]);
    }
    if (3 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx3, terrain.layerNormalTextureIdx3, weights.a, terrainXZ, terrain.layerTileST[3]);
    }

    if (0.0f < weightSum)
    {
        result.albedo = blended.albedo / weightSum;
        result.normalTS = normalize(blended.normalTS / weightSum);
    }
    return result;
}

inline float HardShadow(float3 origin, float3 dir)
{
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE> rq;
    RayDesc ray = { origin, RAY_TMIN, dir, RAY_TMAX };
    rq.TraceRayInline(g_scene, RAY_FLAG_NONE, 0xFF, ray);
    while (rq.Proceed()) {}
    return (rq.CommittedStatus() == COMMITTED_NOTHING) ? 1.0f : 0.0f;
}

[shader("raygeneration")]
void RayGenMain()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;
    float2 ndc = (float2(pixelCoord) + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    float4 worldPos = mul(float4(ndc, 1.0f, 1.0f), g_viewProjInverse); worldPos /= worldPos.w;
    float4 cameraPos = mul(float4(0, 0, 0, 1), g_viewProjInverse); cameraPos /= cameraPos.w;
	
    RayDesc ray = { cameraPos.xyz, RAY_TMIN, normalize(worldPos.xyz - cameraPos.xyz), RAY_TMAX };
    RayPayload payload = { float3(0, 0, 0), 0 };
	
    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
	
    g_output[pixelCoord] = float4(pow(payload.color, (1.0f / 2.2f).xxx), 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    InstanceData inst = g_instanceBuffer[InstanceID()];
    GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + GeometryIndex()];
    MaterialGPUData mat = g_materials[geo.materialIdx];

    StructuredBuffer<Vertex> vBuffer = ResourceDescriptorHeap[inst.vertexBufferIdx];
    Buffer<uint>             iBuffer = ResourceDescriptorHeap[inst.indexBufferIdx];

    uint triIdx = PrimitiveIndex();
    uint i0 = iBuffer[geo.indexBase + triIdx * 3 + 0];
    uint i1 = iBuffer[geo.indexBase + triIdx * 3 + 1];
    uint i2 = iBuffer[geo.indexBase + triIdx * 3 + 2];

    Vertex v0 = vBuffer[i0]; Vertex v1 = vBuffer[i1]; Vertex v2 = vBuffer[i2];
    float3 bary = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
    
    // Safety: Normal Transformation
    float3 normalObj = normalize(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z);
    float3 worldNormal = normalize(mul(normalObj, (float3x3) inst.worldInverse));
    float4 tangentPacked = v0.tangent * bary.x + v1.tangent * bary.y + v2.tangent * bary.z;
    float3 tangent = normalize(mul(tangentPacked.xyz, (float3x3) inst.worldMatrix));
    tangent = normalize(tangent - worldNormal * dot(tangent, worldNormal));
    float3 bitangent = normalize(cross(worldNormal, tangent) * tangentPacked.w);
    
    float2 uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;

    float3 albedo = mat.albedo.rgb;
    if (0 != (mat.materialFlags & MATERIAL_FLAG_TERRAIN_SPLAT))
    {
        TerrainSample terrainSample = SampleTerrainSplat(mat, uv);
        albedo *= terrainSample.albedo;
        worldNormal = normalize(
            tangent * terrainSample.normalTS.x +
            bitangent * terrainSample.normalTS.y +
            worldNormal * terrainSample.normalTS.z
        );
    }
    else if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ALBEDO_MAP))
    {
        Texture2D albedoTex = ResourceDescriptorHeap[mat.albedoTextureIdx];
        albedo *= albedoTex.SampleLevel(g_sampler, uv, 0).rgb;
    }
    if (dot(worldNormal, WorldRayDirection()) > 0.0f) worldNormal = -worldNormal;

    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float shadow = HardShadow(WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + worldNormal * 0.01, lightDir);
    payload.color = albedo * (saturate(dot(worldNormal, lightDir)) * shadow * 2.0 + 0.15);
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    payload.color = float3(0.1, 0.1, 0.1);
}
