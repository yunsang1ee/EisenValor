#ifndef RAYTRACING_TERRAIN_HLSLI
#define RAYTRACING_TERRAIN_HLSLI

#include "RaytracingCommon.h"

struct TerrainSample
{
    float3 albedo;
    float3 normalTS;
    float metallic;
    float roughness;
    float ao;
};

static const uint RAYTRACING_INVALID_TEXTURE_INDEX = 0xffffffffu;

void AccumulateTerrainLayer(
    inout TerrainSample blended,
    inout float weightSum,
    uint albedoTextureIdx,
    uint normalTextureIdx,
    uint ormTextureIdx,
    float2 metallicRoughness,
    float weight,
    float2 terrainXZ,
    float4 tileST,
    SamplerState materialSampler)
{
    if (weight <= 0.0f)
    {
        return;
    }

    float2 layerUV = terrainXZ * tileST.xy + tileST.zw;
    float4 layerAlbedoSample = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (albedoTextureIdx != RAYTRACING_INVALID_TEXTURE_INDEX)
    {
        Texture2D albedoTexture = ResourceDescriptorHeap[albedoTextureIdx];
        layerAlbedoSample = albedoTexture.SampleLevel(materialSampler, layerUV, 0);
    }
    float3 layerAlbedo = layerAlbedoSample.rgb;

    float3 layerNormalTS = float3(0.0f, 0.0f, 1.0f);
    if (normalTextureIdx != RAYTRACING_INVALID_TEXTURE_INDEX)
    {
        Texture2D normalTexture = ResourceDescriptorHeap[normalTextureIdx];
        float2 encodedXY = normalTexture.SampleLevel(materialSampler, layerUV, 0).xy * 2.0f - 1.0f;
        layerNormalTS = float3(encodedXY, sqrt(saturate(1.0f - dot(encodedXY, encodedXY))));
    }

    float layerMetallic = metallicRoughness.x;
    float layerRoughness = metallicRoughness.y;
    float layerAo = 1.0f;
    if (ormTextureIdx != RAYTRACING_INVALID_TEXTURE_INDEX)
    {
        Texture2D ormTexture = ResourceDescriptorHeap[ormTextureIdx];
        float4 p = ormTexture.SampleLevel(materialSampler, layerUV, 0);
        layerMetallic = p.r;
        layerAo = p.g;
        layerRoughness = 1.0f - p.a;
    }
    else
    {
        float useDiffuseAlphaSmoothness = 1.0f - step(0.999f, layerAlbedoSample.a);
        layerRoughness = lerp(layerRoughness, 1.0f - layerAlbedoSample.a, useDiffuseAlphaSmoothness);
    }

    blended.albedo += layerAlbedo * weight;
    blended.normalTS += layerNormalTS * weight;
    blended.metallic += layerMetallic * weight;
    blended.roughness += layerRoughness * weight;
    blended.ao += layerAo * weight;
    weightSum += weight;
}

void AccumulateTerrainLayerPrimary(
    inout TerrainSample blended,
    inout float weightSum,
    uint albedoTextureIdx,
    uint normalTextureIdx,
    float weight,
    float2 terrainXZ,
    float4 tileST,
    SamplerState materialSampler)
{
    if (weight <= 0.0f)
    {
        return;
    }

    float2 layerUV = terrainXZ * tileST.xy + tileST.zw;
    float3 layerAlbedo = 1.0f.xxx;
    if (albedoTextureIdx != RAYTRACING_INVALID_TEXTURE_INDEX)
    {
        Texture2D albedoTexture = ResourceDescriptorHeap[albedoTextureIdx];
        layerAlbedo = albedoTexture.SampleLevel(materialSampler, layerUV, 0).rgb;
    }

    float3 layerNormalTS = float3(0.0f, 0.0f, 1.0f);
    if (normalTextureIdx != RAYTRACING_INVALID_TEXTURE_INDEX)
    {
        Texture2D normalTexture = ResourceDescriptorHeap[normalTextureIdx];
        float2 encodedXY = normalTexture.SampleLevel(materialSampler, layerUV, 0).xy * 2.0f - 1.0f;
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
    uint2 p0 = (uint2) floor(texel);
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

TerrainSample SampleTerrainSplat(
    MaterialGPUData mat,
    float2 terrainUV,
    StructuredBuffer<TerrainSurfaceGPUData> terrainSurfaces,
    SamplerState materialSampler)
{
    TerrainSample result;
    result.albedo = 1.0f.xxx;
    result.normalTS = float3(0.0f, 0.0f, 1.0f);
    result.metallic = 0.0f;
    result.roughness = 1.0f;
    result.ao = 1.0f;

    if (mat.terrainSurfaceIdx == RAYTRACING_INVALID_TEXTURE_INDEX)
    {
        return result;
    }

    TerrainSurfaceGPUData terrain = terrainSurfaces[mat.terrainSurfaceIdx];
    if (terrain.splatTextureIdx == RAYTRACING_INVALID_TEXTURE_INDEX)
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
    blended.metallic = 0.0f;
    blended.roughness = 0.0f;
    blended.ao = 0.0f;
    float weightSum = 0.0f;
    if (0 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx0, terrain.layerNormalTextureIdx0, terrain.layerOrmTextureIdx0, terrain.layerMetallicRoughness[0].xy, weights.r, terrainXZ, terrain.layerTileST[0], materialSampler);
    }
    if (1 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx1, terrain.layerNormalTextureIdx1, terrain.layerOrmTextureIdx1, terrain.layerMetallicRoughness[1].xy, weights.g, terrainXZ, terrain.layerTileST[1], materialSampler);
    }
    if (2 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx2, terrain.layerNormalTextureIdx2, terrain.layerOrmTextureIdx2, terrain.layerMetallicRoughness[2].xy, weights.b, terrainXZ, terrain.layerTileST[2], materialSampler);
    }
    if (3 < layerCount)
    {
        AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx3, terrain.layerNormalTextureIdx3, terrain.layerOrmTextureIdx3, terrain.layerMetallicRoughness[3].xy, weights.a, terrainXZ, terrain.layerTileST[3], materialSampler);
    }

    if (0.0f < weightSum)
    {
        result.albedo = blended.albedo / weightSum;
        result.normalTS = normalize(blended.normalTS / weightSum);
        result.metallic = blended.metallic / weightSum;
        result.roughness = blended.roughness / weightSum;
        result.ao = blended.ao / weightSum;
    }
    return result;
}

TerrainSample SampleTerrainSplatPrimary(
    MaterialGPUData mat,
    float2 terrainUV,
    StructuredBuffer<TerrainSurfaceGPUData> terrainSurfaces,
    SamplerState materialSampler)
{
    TerrainSample result;
    result.albedo = 1.0f.xxx;
    result.normalTS = float3(0.0f, 0.0f, 1.0f);
    result.metallic = 0.0f;
    result.roughness = 1.0f;
    result.ao = 1.0f;

    if (mat.terrainSurfaceIdx == RAYTRACING_INVALID_TEXTURE_INDEX)
    {
        return result;
    }

    TerrainSurfaceGPUData terrain = terrainSurfaces[mat.terrainSurfaceIdx];
    if (terrain.splatTextureIdx == RAYTRACING_INVALID_TEXTURE_INDEX)
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
    blended.metallic = 0.0f;
    blended.roughness = 0.0f;
    blended.ao = 0.0f;
    float weightSum = 0.0f;
    if (0 < layerCount)
    {
        AccumulateTerrainLayerPrimary(blended, weightSum, terrain.layerAlbedoTextureIdx0, terrain.layerNormalTextureIdx0, weights.r, terrainXZ, terrain.layerTileST[0], materialSampler);
    }
    if (1 < layerCount)
    {
        AccumulateTerrainLayerPrimary(blended, weightSum, terrain.layerAlbedoTextureIdx1, terrain.layerNormalTextureIdx1, weights.g, terrainXZ, terrain.layerTileST[1], materialSampler);
    }
    if (2 < layerCount)
    {
        AccumulateTerrainLayerPrimary(blended, weightSum, terrain.layerAlbedoTextureIdx2, terrain.layerNormalTextureIdx2, weights.b, terrainXZ, terrain.layerTileST[2], materialSampler);
    }
    if (3 < layerCount)
    {
        AccumulateTerrainLayerPrimary(blended, weightSum, terrain.layerAlbedoTextureIdx3, terrain.layerNormalTextureIdx3, weights.a, terrainXZ, terrain.layerTileST[3], materialSampler);
    }

    if (0.0f < weightSum)
    {
        result.albedo = blended.albedo / weightSum;
        result.normalTS = normalize(blended.normalTS / weightSum);
    }
    return result;
}

#endif // RAYTRACING_TERRAIN_HLSLI
