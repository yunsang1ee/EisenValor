#ifndef RAYTRACING_NEE_HLSLI
#define RAYTRACING_NEE_HLSLI

#include "RaytracingCommon.h"
#include "RaytracingSampling.hlsli"
#include "RaytracingEnvironment.hlsli"
#include "RaytracingLighting.hlsli"

void BuildRaytracingOrthonormalBasis(float3 normal, out float3 tangent, out float3 bitangent)
{
    float sign = normal.z >= 0.0f ? 1.0f : -1.0f;
    float a = -1.0f / (sign + normal.z);
    float b = normal.x * normal.y * a;
    tangent = float3(1.0f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
    bitangent = float3(b, sign + normal.y * normal.y * a, -normal.y);
}

float GetDirectionalAreaSunSolidAngle(float angularRadius)
{
    float clampedAngularRadius = max(angularRadius, 0.0f);
    return max(2.0f * PI * (1.0f - cos(clampedAngularRadius)), EPSILON);
}

float GetDirectionalAreaSunPdf(float angularRadius)
{
    return rcp(GetDirectionalAreaSunSolidAngle(angularRadius));
}

float3 SampleDirectionalAreaSun(float3 lightDir, float angularRadius, inout uint seed, out float lightPdf)
{
    float clampedAngularRadius = max(angularRadius, 0.0f);
    float cosMaxTheta = cos(clampedAngularRadius);
    float u0 = RandomValue(seed);
    float u1 = RandomValue(seed);
    float cosTheta = lerp(cosMaxTheta, 1.0f, u0);
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2.0f * PI * u1;

    float3 tangent;
    float3 bitangent;
    float3 axis = normalize(lightDir);
    BuildRaytracingOrthonormalBasis(axis, tangent, bitangent);

    lightPdf = GetDirectionalAreaSunPdf(clampedAngularRadius);
    return normalize(axis * cosTheta + (cos(phi) * tangent + sin(phi) * bitangent) * sinTheta);
}

inline float ShadowVisibility(
    RaytracingAccelerationStructure accel,
    float3 origin,
    float3 dir,
    float tMin,
    float tMax,
    uint mask)
{
    RayQuery <
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_FORCE_OPAQUE |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
    > rq;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = tMin;
    ray.TMax = tMax;

    rq.TraceRayInline(accel, RAY_FLAG_NONE, mask, ray);

    while (rq.Proceed())
    {
        if (rq.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            rq.CommitNonOpaqueTriangleHit();
        }
    }

    return (rq.CommittedStatus() == COMMITTED_NOTHING) ? 1.0f : 0.0f;
}

float SoftShadowVisibilityDirLight(
    RaytracingAccelerationStructure accel,
    float3 origin,
    float3 normal,
    float3 lightDir,
    float angularRadius,
    uint mask,
    uint sampleCount,
    inout uint seed)
{
    uint resolvedSampleCount = max(sampleCount, 1u);
    float visibility = 0.0f;

    [loop]
    for (uint i = 0; i < resolvedSampleCount; ++i)
    {
        float lightPdf;
        float3 jitteredDir = SampleDirectionalAreaSun(lightDir, angularRadius, seed, lightPdf);
        if (dot(jitteredDir, normal) <= 0.0f)
        {
            continue;
        }

        visibility += ShadowVisibility(accel, origin, jitteredDir, RAY_TMIN, RAY_TMAX, mask);
    }

    return visibility / float(resolvedSampleCount);
}

float3 EvaluateEnvironmentSunNEE(
    RaytracingAccelerationStructure accel,
    uint environmentMode,
    float3 hitPos,
    float3 geometricNormal,
    float3 shadingNormal,
    float3 viewDir,
    float3 albedo,
    float metallic,
    float roughness,
    float ao,
    float3 F0,
    uint visibilitySampleCount,
    out float lightPdf,
    out float3 sampledLightDirection,
    out float3 sampledIncidentRadiance,
    inout uint rngSeed)
{
    float3 sunAxis = GetEnvironmentSunDirection();
    float angularRadius = GetEnvironmentSunAngularRadius(environmentMode);
    lightPdf = GetDirectionalAreaSunPdf(angularRadius);
    sampledLightDirection = sunAxis;
    sampledIncidentRadiance = 0.0f.xxx;

    float3 sunIntensity = GetEnvironmentSunRadiance(environmentMode);
    if (max(max(sunIntensity.x, sunIntensity.y), sunIntensity.z) <= 0.0f)
    {
        return 0.0f.xxx;
    }

    uint resolvedSampleCount = max(visibilitySampleCount, 1u);
    float3 shadowOrigin = hitPos + geometricNormal * 0.01f;
    float3 lighting = 0.0f.xxx;
    float3 sunRadiance = sunIntensity * lightPdf;

    [loop]
    for (uint i = 0u; i < resolvedSampleCount; ++i)
    {
        float samplePdf;
        float3 lightDir = SampleDirectionalAreaSun(sunAxis, angularRadius, rngSeed, samplePdf);
        if (i == 0u)
        {
            sampledLightDirection = lightDir;
        }

        float NdotL = max(dot(shadingNormal, lightDir), 0.0f);
        float NdotV = max(dot(shadingNormal, viewDir), 0.0f);
        if (NdotL <= 0.0f || NdotV <= 0.0f)
        {
            continue;
        }

        float3 H = normalize(viewDir + lightDir);
        float NDF = DistributionGGX(shadingNormal, H, roughness);
        float G = GeometrySmith(shadingNormal, viewDir, lightDir, roughness);
        float3 F = FresnelSchlick(saturate(dot(H, viewDir)), F0);
        float3 specular = (NDF * G * F) / max(4.0f * NdotV * NdotL, EPSILON);
        float3 kD = (1.0f - F) * (1.0f - metallic);

        float visibility = ShadowVisibility(accel, shadowOrigin, lightDir, RAY_TMIN, RAY_TMAX, 0xFF);
        if (i == 0u)
        {
            sampledIncidentRadiance = sunRadiance * visibility;
        }
        lighting += (kD * albedo * ao / PI + specular) * sunRadiance * NdotL * visibility / max(samplePdf, EPSILON);
    }

    return lighting / float(resolvedSampleCount);
}

float3 EvaluateEnvironmentSunNEE(
    RaytracingAccelerationStructure accel,
    uint environmentMode,
    float3 hitPos,
    float3 geometricNormal,
    float3 shadingNormal,
    float3 viewDir,
    float3 albedo,
    float metallic,
    float roughness,
    float ao,
    float3 F0,
    uint visibilitySampleCount,
    out float lightPdf,
    inout uint rngSeed)
{
    float3 sampledLightDirection;
    float3 sampledIncidentRadiance;
    return EvaluateEnvironmentSunNEE(
        accel,
        environmentMode,
        hitPos,
        geometricNormal,
        shadingNormal,
        viewDir,
        albedo,
        metallic,
        roughness,
        ao,
        F0,
        visibilitySampleCount,
        lightPdf,
        sampledLightDirection,
        sampledIncidentRadiance,
        rngSeed
    );
}

float3 EvaluateEnvironmentSunNEE(
    RaytracingAccelerationStructure accel,
    uint environmentMode,
    float3 hitPos,
    float3 geometricNormal,
    float3 shadingNormal,
    float3 viewDir,
    float3 albedo,
    float metallic,
    float roughness,
    float ao,
    float3 F0,
    uint visibilitySampleCount,
    inout uint rngSeed)
{
    float lightPdf;
    return EvaluateEnvironmentSunNEE(
        accel,
        environmentMode,
        hitPos,
        geometricNormal,
        shadingNormal,
        viewDir,
        albedo,
        metallic,
        roughness,
        ao,
        F0,
        visibilitySampleCount,
        lightPdf,
        rngSeed);
}

#endif // RAYTRACING_NEE_HLSLI
