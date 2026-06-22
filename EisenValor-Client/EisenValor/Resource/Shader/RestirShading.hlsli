#ifndef RESTIR_SHADING_HLSLI
#define RESTIR_SHADING_HLSLI

#include "RaytracingCommon.h"
#include "RestirReservoir.hlsli"
#include "RaytracingTerrain.hlsli"
#include "RaytracingNormal.hlsli"
#include "RaytracingLighting.hlsli"

// The including shader owns the register layout for g_instanceBuffer, g_geoTable,
// g_materials, g_terrainSurfaces, and g_sampler.

struct RestirSurface
{
    float3 position;
    float3 faceNormal;
    float3 geometricNormal;
    float3 normal;
    float3 shadingNormal;
    float2 uv;
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
    MaterialGPUData material;
};

float3 RestirStrengthenTangentSpaceNormal(float3 normalTS, float strength)
{
    return normalize(float3(normalTS.xy * strength, max(normalTS.z, 0.0f)));
}

float RestirComputeSpecularProbability(RestirSurface surface, float3 viewDirection)
{
    float3 V = SafeNormalizeRay(viewDirection, surface.shadingNormal);
    float NdotV = max(dot(surface.shadingNormal, V), 0.0f);
    float3 F0 = lerp(0.04f.xxx, surface.albedo, surface.metallic);
    float3 F = FresnelSchlick(NdotV, F0);
    float3 diffuseWeight = (1.0f - F) * (1.0f - surface.metallic) * surface.albedo;
    float diffuseContribution = dot(diffuseWeight, float3(0.299f, 0.587f, 0.114f));
    float specularContribution = dot(F, float3(0.299f, 0.587f, 0.114f));
    float totalContribution = diffuseContribution + specularContribution;
    float probability = totalContribution > 0.0001f ? specularContribution / totalContribution : 0.0f;
    return clamp(probability, 0.01f, 0.99f);
}

float3 RestirEvalBSDF(
    RestirSurface surface,
    float3 viewDirection,
    float3 lightDirection,
    uint lobeFlags)
{
    float3 V = SafeNormalizeRay(viewDirection, surface.shadingNormal);
    float3 L = SafeNormalizeRay(lightDirection, surface.shadingNormal);
    float NdotV = max(dot(surface.shadingNormal, V), 0.0f);
    float NdotL = max(dot(surface.shadingNormal, L), 0.0f);
    if (NdotV <= 0.0f || NdotL <= 0.0f)
    {
        return 0.0f.xxx;
    }

    bool evaluateDiffuse = 0u != (lobeFlags & RESTIR_BSDF_LOBE_DIFFUSE);
    bool evaluateSpecular = 0u != (lobeFlags & RESTIR_BSDF_LOBE_SPECULAR);
    float3 F0 = lerp(0.04f.xxx, surface.albedo, surface.metallic);
    float3 H = SafeNormalizeRay(V + L, surface.shadingNormal);
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    float3 result = 0.0f.xxx;

    if (evaluateDiffuse)
    {
        float3 kD = (1.0f - F) * (1.0f - surface.metallic);
        result += kD * surface.albedo * surface.ao * (NdotL / PI);
    }

    if (evaluateSpecular)
    {
        if (surface.roughness < 0.15f)
        {
            float3 mirrorDirection = reflect(-V, surface.shadingNormal);
            if (dot(mirrorDirection, L) > 0.999f)
            {
                result += FresnelSchlick(NdotV, F0);
            }
        }
        else
        {
            float ndf = DistributionGGX(surface.shadingNormal, H, surface.roughness);
            float geometry = GeometrySmith(surface.shadingNormal, V, L, surface.roughness);
            float3 bsdf = ndf * geometry * F / max(4.0f * NdotV * NdotL, EPSILON);
            result += bsdf * NdotL;
        }
    }

    return result;
}

float RestirEvalPdfBSDF(
    RestirSurface surface,
    float3 viewDirection,
    float3 lightDirection,
    uint lobeFlags)
{
    float3 V = SafeNormalizeRay(viewDirection, surface.shadingNormal);
    float3 L = SafeNormalizeRay(lightDirection, surface.shadingNormal);
    float NdotV = max(dot(surface.shadingNormal, V), 0.0f);
    float NdotL = max(dot(surface.shadingNormal, L), 0.0f);
    if (NdotV <= 0.0f || NdotL <= 0.0f)
    {
        return 0.0f;
    }

    float specularProbability = RestirComputeSpecularProbability(surface, V);
    float pdf = 0.0f;
    if (0u != (lobeFlags & RESTIR_BSDF_LOBE_DIFFUSE))
    {
        pdf += (1.0f - specularProbability) * NdotL / PI;
    }
    if (0u != (lobeFlags & RESTIR_BSDF_LOBE_SPECULAR))
    {
        if (surface.roughness < 0.15f)
        {
            float3 mirrorDirection = reflect(-V, surface.shadingNormal);
            pdf += dot(mirrorDirection, L) > 0.999f ? specularProbability : 0.0f;
        }
        else
        {
            float3 H = SafeNormalizeRay(V + L, surface.shadingNormal);
            pdf += specularProbability * GGX_PDF(
                surface.shadingNormal,
                H,
                V,
                L,
                surface.roughness
            );
        }
    }
    return max(pdf, 0.0f);
}

void RestirBuildSurface(
    InstanceData inst,
    MaterialGPUData mat,
    Vertex v0,
    Vertex v1,
    Vertex v2,
    float3 bary,
    float3 position,
    float shadingNormalStrength,
    out RestirSurface surface)
{
    surface = (RestirSurface)0;
    surface.position = position;
    surface.material = mat;

    float3 normalObj = SafeNormalizeRay(
        v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z,
        float3(0.0f, 1.0f, 0.0f)
    );
    surface.geometricNormal = SafeNormalizeRay(
        mul(normalObj, (float3x3)inst.worldInverse),
        float3(0.0f, 1.0f, 0.0f)
    );
    float3 p0 = mul(float4(v0.position, 1.0f), inst.worldMatrix).xyz;
    float3 p1 = mul(float4(v1.position, 1.0f), inst.worldMatrix).xyz;
    float3 p2 = mul(float4(v2.position, 1.0f), inst.worldMatrix).xyz;
    surface.faceNormal = SafeNormalizeRay(
        cross(p1 - p0, p2 - p0),
        surface.geometricNormal
    );
    surface.normal = surface.geometricNormal;
    surface.shadingNormal = surface.geometricNormal;

    float4 tangentPacked = v0.tangent * bary.x + v1.tangent * bary.y + v2.tangent * bary.z;
    float3 tangentCandidate = mul(tangentPacked.xyz, (float3x3)inst.worldMatrix);
    float3 tangent;
    float3 bitangent;
    BuildSafeTangentFrame(
        surface.geometricNormal,
        tangentCandidate,
        tangentPacked.w,
        tangent,
        bitangent
    );

    surface.uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;
    TerrainSample terrainSample = (TerrainSample)0;
    bool useTerrainSplat = 0u != (mat.materialFlags & MATERIAL_FLAG_TERRAIN_SPLAT);
    if (useTerrainSplat)
    {
        terrainSample = SampleTerrainSplat(mat, surface.uv, g_terrainSurfaces, g_sampler);
        float3 shadingNormalTS = RestirStrengthenTangentSpaceNormal(
            terrainSample.normalTS,
            shadingNormalStrength
        );
        surface.normal = TangentNormalToWorld(
            terrainSample.normalTS,
            tangent,
            bitangent,
            surface.geometricNormal
        );
        surface.shadingNormal = normalize(
            tangent * shadingNormalTS.x +
            bitangent * shadingNormalTS.y +
            surface.geometricNormal * shadingNormalTS.z
        );
    }
    else if (0u != (mat.materialFlags & MATERIAL_FLAG_USE_NORMAL_MAP))
    {
        Texture2D normalTexture = ResourceDescriptorHeap[mat.normalTextureIdx];
        float2 encodedXY = normalTexture.SampleLevel(g_sampler, surface.uv, 0).xy * 2.0f - 1.0f;
        float3 normalSample = float3(encodedXY, sqrt(saturate(1.0f - dot(encodedXY, encodedXY))));
        float3 shadingNormalSample = RestirStrengthenTangentSpaceNormal(
            normalSample,
            shadingNormalStrength
        );
        surface.normal = TangentNormalToWorld(
            normalSample,
            tangent,
            bitangent,
            surface.geometricNormal
        );
        surface.shadingNormal = normalize(
            tangent * shadingNormalSample.x +
            bitangent * shadingNormalSample.y +
            surface.geometricNormal * shadingNormalSample.z
        );
    }

    if (dot(surface.normal, surface.geometricNormal) < 0.0f)
    {
        surface.normal = surface.geometricNormal;
    }
    if (dot(surface.shadingNormal, surface.geometricNormal) < 0.0f)
    {
        surface.shadingNormal = surface.geometricNormal;
    }

    surface.albedo = mat.albedo.rgb;
    if (useTerrainSplat)
    {
        surface.albedo *= terrainSample.albedo;
    }
    else if (0u != (mat.materialFlags & MATERIAL_FLAG_USE_ALBEDO_MAP))
    {
        Texture2D albedoTexture = ResourceDescriptorHeap[mat.albedoTextureIdx];
        surface.albedo *= albedoTexture.SampleLevel(g_sampler, surface.uv, 0).rgb;
    }

    surface.metallic = mat.metallic;
    surface.roughness = mat.roughness;
    surface.ao = 1.0f;
    if (useTerrainSplat)
    {
        surface.metallic = terrainSample.metallic;
        surface.roughness = terrainSample.roughness;
        surface.ao = terrainSample.ao;
    }
    else if (0u != (mat.materialFlags & MATERIAL_FLAG_USE_ORM_MAP))
    {
        Texture2D ormTexture = ResourceDescriptorHeap[mat.ormTextureIdx];
        float4 orm = ormTexture.SampleLevel(g_sampler, surface.uv, 0);
        if (0u != (mat.materialFlags & MATERIAL_FLAG_UNITY_PACKING))
        {
            surface.metallic = orm.r;
            surface.ao = orm.g;
            surface.roughness = 1.0f - orm.a;
        }
        else
        {
            surface.ao = orm.r;
            surface.metallic = orm.b;
            surface.roughness = orm.g;
        }
    }
    surface.roughness = max(surface.roughness, 0.04f);
}

bool RestirLoadSurface(
    uint instanceIndex,
    uint geometryIndex,
    uint primitiveIndex,
    uint packedBarycentrics,
    float shadingNormalStrength,
    out RestirSurface surface)
{
    InstanceData inst = g_instanceBuffer[instanceIndex];
    GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + geometryIndex];
    MaterialGPUData mat = g_materials[geo.materialIdx];
    StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[inst.vertexBufferIdx];
    Buffer<uint> indexBuffer = ResourceDescriptorHeap[inst.indexBufferIdx];

    uint i0 = indexBuffer[geo.indexBase + primitiveIndex * 3u + 0u];
    uint i1 = indexBuffer[geo.indexBase + primitiveIndex * 3u + 1u];
    uint i2 = indexBuffer[geo.indexBase + primitiveIndex * 3u + 2u];
    Vertex v0 = vertexBuffer[i0];
    Vertex v1 = vertexBuffer[i1];
    Vertex v2 = vertexBuffer[i2];

    float2 packedBary = RestirUnpackBarycentrics(packedBarycentrics);
    float3 bary = float3(1.0f - packedBary.x - packedBary.y, packedBary.x, packedBary.y);
    float3 positionObj = v0.position * bary.x + v1.position * bary.y + v2.position * bary.z;
    float3 position = mul(float4(positionObj, 1.0f), inst.worldMatrix).xyz;
    RestirBuildSurface(
        inst,
        mat,
        v0,
        v1,
        v2,
        bary,
        position,
        shadingNormalStrength,
        surface
    );
    return true;
}

#endif // RESTIR_SHADING_HLSLI
