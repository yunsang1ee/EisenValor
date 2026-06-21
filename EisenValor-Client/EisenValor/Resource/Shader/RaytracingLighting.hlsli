#ifndef RAYTRACING_LIGHTING_HLSLI
#define RAYTRACING_LIGHTING_HLSLI

#include "RaytracingCommon.h"

#ifdef RAYTRACING_ENABLE_GGX_SAMPLING
#include "RaytracingSampling.hlsli"
#endif

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / max(denom, EPSILON);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / max(denom, EPSILON);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

#ifdef RAYTRACING_ENABLE_GGX_SAMPLING
float3 SampleGGX(float3 N, float3 V, float roughness, inout uint seed)
{
    float a = roughness * roughness;

    float u1 = RandomValue(seed);
    float u2 = RandomValue(seed);

    float theta = atan(a * sqrt(u1) / sqrt(1.0f - u1 + EPSILON));
    float phi = 2.0f * PI * u2;

    float3 H_local;
    H_local.x = sin(theta) * cos(phi);
    H_local.y = cos(theta);
    H_local.z = sin(theta) * sin(phi);

    float3 up = abs(N.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 H = normalize(tangent * H_local.x + N * H_local.y + bitangent * H_local.z);
    return reflect(-V, H);
}

float3 SampleGGXReflection(float3 V, float3 N, float roughness, inout uint seed)
{
    return SampleGGX(N, V, roughness, seed);
}
#endif

float GGX_PDF(float3 N, float3 H, float3 V, float3 L, float roughness)
{
    float NdotH = max(dot(N, H), 0.0f);
    float VdotH = max(dot(V, H), 0.0f);

    float D = DistributionGGX(N, H, roughness);
    return (D * NdotH) / max(4.0f * VdotH, EPSILON);
}

#endif // RAYTRACING_LIGHTING_HLSLI
