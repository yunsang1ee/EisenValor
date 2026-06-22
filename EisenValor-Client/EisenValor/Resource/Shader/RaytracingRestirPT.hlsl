#define HLSL
#include "RaytracingCommon.h"
#include "RaytracingPayload.hlsli"
#include "RaytracingMaterialEmission.hlsli"
#include "RaytracingEnvironment.hlsli"
#include "RaytracingPostProcess.hlsli"
#include "RaytracingSampling.hlsli"
#include "RaytracingLighting.hlsli"
#include "RaytracingNEE.hlsli"

typedef RestirRayPayload RayPayload;

#define RESTIR_ENABLE_PAYLOAD_HELPERS 1
#include "RestirReservoir.hlsli"

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);
RWStructuredBuffer<RestirPrimaryHit> g_restirPrimaryHitCurrent : register(u1, space0);
RWStructuredBuffer<RestirReservoir> g_restirReservoirInitial : register(u2, space0);
RWTexture2D<float2> g_restirMotionVector : register(u3, space0);
RWTexture2D<float> g_restirLinearDepth : register(u4, space0);
RWTexture2D<float4> g_restirDiffuseAlbedo : register(u5, space0);
RWTexture2D<float4> g_restirSpecularAlbedo : register(u6, space0);
RWTexture2D<float4> g_restirNormalRoughness : register(u7, space0);

cbuffer CameraConstants : register(b0, space0)
{
    float4x4 g_viewProjInverse;
    float2 g_cameraJitterPixels;
    float2 g_cameraConstantsPad0;
};

StructuredBuffer<InstanceData> g_instanceBuffer : register(t1, space0);
StructuredBuffer<MaterialGPUData> g_materials : register(t2, space0);
StructuredBuffer<GeoInfo> g_geoTable : register(t3, space0);
StructuredBuffer<TerrainSurfaceGPUData> g_terrainSurfaces : register(t4, space0);
StructuredBuffer<RestirEmissiveLightData> g_restirEmissiveLights : register(t5, space0);

cbuffer RaytracingFrameConstants : register(b2, space0)
{
    uint g_frameSeed;
    uint g_emissionViewMode;
    uint g_environmentMode;
    uint g_frameConstantsPad0;
};

cbuffer RestirCandidateConstants : register(b3, space0)
{
    uint g_restirCandidateEnabled;
    uint g_restirScreenWidth;
    uint g_restirScreenHeight;
    float g_restirCameraNearZ;
    uint g_restirEmissiveLightCount;
    float g_restirEmissiveLightWeightSum;
    uint g_restirCandidatePad1;
    uint g_restirCandidatePad2;
};

cbuffer RestirCandidateCameraConstants : register(b4, space0)
{
    float4x4 g_previousViewProj;
};

SamplerState g_sampler : register(s0, space0);

#include "RestirShading.hlsli"

static const uint MAX_RECURSION_DEPTH = 6;
static const uint SPP = 1;
static const uint INVALID_TEXTURE_INDEX = 0xffffffffu;
static const float PT_OUTPUT_EXPOSURE = 0.75f;
static const float PT_BLOOM_THRESHOLD = 1.0f;
static const float PT_BLOOM_INTENSITY = 0.75f;
static const float PT_VISIBLE_NORMAL_STRENGTH = RESTIR_STYLIZED_NORMAL_STRENGTH;

// 0: off, 1: shading normal, 2: geometric normal, 3: NdotV(shading/geom), 4: albedo, 5: metallic/roughness/ao.
static const uint PT_DEBUG_VIEW = 0;

void RestirSetPayloadRcIrradiance(inout RayPayload payload, float3 irradiance)
{
    RestirPackRadianceHalf3(
        irradiance,
        payload.packedRcVertexIrradianceXY,
        payload.packedRcVertexIrradianceZ
    );
}

void RestirSetPayloadLobe(inout RayPayload payload, bool beforeRc, uint lobeFlags, bool isDelta)
{
    uint lobeShift = beforeRc ? RESTIR_METADATA_LOBE_BEFORE_SHIFT : RESTIR_METADATA_LOBE_AFTER_SHIFT;
    payload.pathFlags = RestirSetMetadataLobe(payload.pathFlags, lobeShift, lobeFlags);
    if (isDelta)
    {
        payload.pathFlags |= beforeRc ? RESTIR_METADATA_DELTA_BEFORE_RC : RESTIR_METADATA_DELTA_AFTER_RC;
    }
}

float3 CosineHemisphere(float3 normal, inout uint seed)
{
    float u1 = RandomValue(seed);
    float u2 = RandomValue(seed);

    float r = sqrt(u1);
    float theta = 2.0f * PI * u2;

    float x = r * cos(theta);
    float z = r * sin(theta);
    float y = sqrt(max(0.0f, 1.0f - u1));

    float3 w = normal;
    float3 u = normalize(cross(abs(w.x) > 0.1f ? float3(0, 1, 0) : float3(1, 0, 0), w));
    float3 v = cross(w, u);

    return normalize(u * x + v * z + w * y);
}

bool TryClipToUv(float4 clip, out float2 uv)
{
    uv = 0.0f.xx;
    if (clip.w <= EPSILON)
    {
        return false;
    }

    float2 ndc = clip.xy / clip.w;
    uv = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);
    return true;
}

float RestirComputeLinearDepth(float3 worldPosition)
{
    float4 nearCenter = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), g_viewProjInverse);
    nearCenter /= nearCenter.w;

    float4 farCenter = mul(float4(0.0f, 0.0f, 1.0f, 1.0f), g_viewProjInverse);
    farCenter /= farCenter.w;

    float3 cameraForward = normalize(farCenter.xyz - nearCenter.xyz);
    return max(0.0f, g_restirCameraNearZ + dot(worldPosition - nearCenter.xyz, cameraForward));
}

uint2 RestirPixelCoordFromIndex(uint pixelIndex)
{
    return uint2(pixelIndex % g_restirScreenWidth, pixelIndex / g_restirScreenWidth);
}

void RestirWriteInvalidPrimaryOutputs(uint pixelIndex)
{
    if (pixelIndex == 0xffffffffu || g_restirScreenWidth == 0u || g_restirScreenHeight == 0u)
    {
        return;
    }

    uint2 pixelCoord = RestirPixelCoordFromIndex(pixelIndex);
    g_restirPrimaryHitCurrent[pixelIndex] = RestirMakeInvalidPrimaryHit();
    g_restirMotionVector[pixelCoord] = 0.0f.xx;
    g_restirLinearDepth[pixelCoord] = -1.0f;
    g_restirDiffuseAlbedo[pixelCoord] = 0.0f.xxxx;
    g_restirSpecularAlbedo[pixelCoord] = 0.0f.xxxx;
    g_restirNormalRoughness[pixelCoord] = float4(0.0f, 0.0f, 0.0f, 1.0f);
}

void RestirWritePrimaryOutputs(
    uint pixelIndex,
    float3 hitPos,
    float hitDistance,
    float3 geometricNormal,
    float3 shadingNormal,
    float3 albedo,
    float metallic,
    float roughness,
    InstanceData inst,
    MaterialGPUData mat,
    GeoInfo geo,
    uint geometryIndex,
    uint primitiveIndex,
    float2 barycentrics)
{
    if (pixelIndex == 0xffffffffu || g_restirScreenWidth == 0u || g_restirScreenHeight == 0u)
    {
        return;
    }

    RestirPrimaryHit primaryHit;
    primaryHit.positionDistance = float4(hitPos, hitDistance);
    primaryHit.packedNormal = RestirPackNormalOct16(geometricNormal);
    primaryHit.packedRoughnessFlags =
        RestirPackPrimaryHitRoughnessFlags(roughness, RESTIR_PRIMARY_HIT_VALID);
    primaryHit.instanceId = inst.instanceID;
    primaryHit.materialId = mat.stableMaterialId;
    primaryHit.geometryId = geo.stableGeometryId;
    primaryHit.geometryIndex = geometryIndex;
    primaryHit.primitiveIndex = primitiveIndex;
    primaryHit.barycentrics = RestirPackBarycentrics(barycentrics);
    g_restirPrimaryHitCurrent[pixelIndex] = primaryHit;

    uint2 pixelCoord = RestirPixelCoordFromIndex(pixelIndex);
    float2 motionVector = 0.0f.xx;
    float4 previousClip = mul(float4(hitPos, 1.0f), g_previousViewProj);
    float2 previousUv;
    if (TryClipToUv(previousClip, previousUv))
    {
        float2 currentUv = (float2(pixelCoord) + 0.5f) / float2(g_restirScreenWidth, g_restirScreenHeight);
        float2 jitterUv = g_cameraJitterPixels / float2(g_restirScreenWidth, g_restirScreenHeight);
        motionVector = previousUv - currentUv - jitterUv;
    }

    g_restirMotionVector[pixelCoord] = motionVector;
    g_restirLinearDepth[pixelCoord] = RestirComputeLinearDepth(hitPos);
    g_restirDiffuseAlbedo[pixelCoord] = float4(albedo * (1.0f - metallic), 1.0f);
    g_restirSpecularAlbedo[pixelCoord] = float4(lerp(0.04f.xxx, albedo, metallic), 1.0f);
    g_restirNormalRoughness[pixelCoord] = float4(normalize(shadingNormal), roughness);
}

bool UsePhysicalRenderingMode()
{
    return EMISSION_VIEW_PHYSICAL == g_emissionViewMode;
}

float RestirGetEmissiveLightWeight(RestirEmissiveLightData light)
{
    return max(light.selectionWeight, 0.0f);
}

float RestirGetEmissiveLightWeightSum()
{
    return max(g_restirEmissiveLightWeightSum, 0.0f);
}

bool RestirSelectEmissiveLight(inout uint rngSeed, out uint lightIndex, out float selectedWeight, out float weightSum)
{
    lightIndex = 0xffffffffu;
    selectedWeight = 0.0f;
    weightSum = RestirGetEmissiveLightWeightSum();
    if (g_restirEmissiveLightCount == 0u || weightSum <= EPSILON)
    {
        return false;
    }

    float r = RandomValue(rngSeed) * weightSum;
    float prefix = 0.0f;
    [loop]
    for (uint i = 0u; i < g_restirEmissiveLightCount; ++i)
    {
        float weight = RestirGetEmissiveLightWeight(g_restirEmissiveLights[i]);
        prefix += weight;
        if (r <= prefix)
        {
            lightIndex = i;
            selectedWeight = weight;
            return weight > 0.0f;
        }
    }

    lightIndex = g_restirEmissiveLightCount - 1u;
    selectedWeight = RestirGetEmissiveLightWeight(g_restirEmissiveLights[lightIndex]);
    return selectedWeight > 0.0f;
}

bool RestirComputeTriangleGeometry(float3 p0, float3 p1, float3 p2, out float area, out float3 normal)
{
    float3 edge01 = p1 - p0;
    float3 edge02 = p2 - p0;
    float3 areaVector = cross(edge01, edge02);
    float doubleArea = length(areaVector);
    if (doubleArea <= EPSILON)
    {
        area = 0.0f;
        normal = float3(0.0f, 1.0f, 0.0f);
        return false;
    }

    area = 0.5f * doubleArea;
    normal = areaVector / doubleArea;
    return true;
}

float RestirComputeEmissiveHitNeePdfSolidAngle(
    InstanceData inst,
    GeoInfo geo,
    MaterialGPUData mat,
    Vertex v0,
    Vertex v1,
    Vertex v2,
    float3 hitPos,
    float3 rayOrigin)
{
    if (geo.emissiveEntryIdx == 0xffffffffu || geo.emissiveEntryIdx >= g_restirEmissiveLightCount)
    {
        return 0.0f;
    }

    RestirEmissiveLightData light = g_restirEmissiveLights[geo.emissiveEntryIdx];
    if (light.triangleCount == 0u)
    {
        return 0.0f;
    }

    float weightSum = RestirGetEmissiveLightWeightSum();
    float selectedWeight = RestirGetEmissiveLightWeight(light);
    if (weightSum <= EPSILON || selectedWeight <= 0.0f)
    {
        return 0.0f;
    }

    float3 p0 = mul(float4(v0.position, 1.0f), inst.worldMatrix).xyz;
    float3 p1 = mul(float4(v1.position, 1.0f), inst.worldMatrix).xyz;
    float3 p2 = mul(float4(v2.position, 1.0f), inst.worldMatrix).xyz;
    float area;
    float3 lightNormal;
    if (!RestirComputeTriangleGeometry(p0, p1, p2, area, lightNormal))
    {
        return 0.0f;
    }

    float3 toLight = hitPos - rayOrigin;
    float distanceSq = dot(toLight, toLight);
    if (distanceSq <= EPSILON)
    {
        return 0.0f;
    }

    float3 lightDir = toLight * rsqrt(distanceSq);
    float rawLightCos = dot(lightNormal, -lightDir);
    float lightCos = (0 != (mat.materialFlags & MATERIAL_FLAG_DOUBLE_SIDED)) ? abs(rawLightCos) : max(rawLightCos, 0.0f);
    if (lightCos <= EPSILON)
    {
        return 0.0f;
    }

    float entryPdf = selectedWeight / weightSum;
    float areaPdf = entryPdf / max((float)light.triangleCount * area, EPSILON);
    return areaPdf * distanceSq / max(lightCos, EPSILON);
}

bool EvaluateRestirEmissiveNEE(
    RestirSurface surface,
    float3 viewDir,
    inout uint rngSeed,
    out float3 contribution,
    out float lightPdf,
    out uint lightInstanceId,
    out uint lightInstanceGeneration,
    out uint lightGeometryIndex,
    out uint lightPrimitiveIndex,
    out uint lightBarycentrics,
    out float3 rcIrradiance,
    out float sourceBsdfPdf,
    out float sourceGeometry)
{
    float3 hitPos = surface.position;
    float3 geometricNormal = surface.geometricNormal;
    float3 shadingNormal = surface.shadingNormal;
    contribution = 0.0f.xxx;
    lightPdf = 0.0f;
    lightInstanceId = 0xffffffffu;
    lightInstanceGeneration = 0u;
    lightGeometryIndex = 0xffffffffu;
    lightPrimitiveIndex = 0xffffffffu;
    lightBarycentrics = 0u;
    rcIrradiance = 0.0f.xxx;
    sourceBsdfPdf = 0.0f;
    sourceGeometry = 0.0f;

    uint lightIndex;
    float selectedWeight;
    float weightSum;
    if (!RestirSelectEmissiveLight(rngSeed, lightIndex, selectedWeight, weightSum))
    {
        return false;
    }

    RestirEmissiveLightData light = g_restirEmissiveLights[lightIndex];
    if (light.triangleCount == 0u)
    {
        return false;
    }

    InstanceData lightInst = g_instanceBuffer[light.instanceIndex];
    GeoInfo lightGeo = g_geoTable[lightInst.geoInfoBaseIdx + light.geometryIndex];
    MaterialGPUData lightMat = g_materials[lightGeo.materialIdx];
    StructuredBuffer<Vertex> lightVB = ResourceDescriptorHeap[lightInst.vertexBufferIdx];
    Buffer<uint> lightIB = ResourceDescriptorHeap[lightInst.indexBufferIdx];

    uint triLocalIndex = min((uint)(RandomValue(rngSeed) * (float)light.triangleCount), light.triangleCount - 1u);
    uint i0 = lightIB[lightGeo.indexBase + triLocalIndex * 3u + 0u];
    uint i1 = lightIB[lightGeo.indexBase + triLocalIndex * 3u + 1u];
    uint i2 = lightIB[lightGeo.indexBase + triLocalIndex * 3u + 2u];

    Vertex lv0 = lightVB[i0];
    Vertex lv1 = lightVB[i1];
    Vertex lv2 = lightVB[i2];

    float3 p0 = mul(float4(lv0.position, 1.0f), lightInst.worldMatrix).xyz;
    float3 p1 = mul(float4(lv1.position, 1.0f), lightInst.worldMatrix).xyz;
    float3 p2 = mul(float4(lv2.position, 1.0f), lightInst.worldMatrix).xyz;
    float area;
    float3 lightNormal;
    if (!RestirComputeTriangleGeometry(p0, p1, p2, area, lightNormal))
    {
        return false;
    }

    float sqrtU = sqrt(RandomValue(rngSeed));
    float v = RandomValue(rngSeed);
    float3 bary = float3(1.0f - sqrtU, sqrtU * (1.0f - v), sqrtU * v);
    float3 lightPos = p0 * bary.x + p1 * bary.y + p2 * bary.z;
    float2 lightUv = lv0.uv * bary.x + lv1.uv * bary.y + lv2.uv * bary.z;
    float3 Le = EvaluateMaterialEmission(lightMat, lightUv, g_sampler);
    lightInstanceId = lightInst.instanceID;
    lightInstanceGeneration = lightInst.generation;
    lightGeometryIndex = light.geometryIndex;
    lightPrimitiveIndex = triLocalIndex;
    lightBarycentrics = RestirPackBarycentrics(bary.yz);
    if (max(max(Le.x, Le.y), Le.z) <= 0.0f)
    {
        return true;
    }

    float3 toLight = lightPos - hitPos;
    float distanceSq = dot(toLight, toLight);
    if (distanceSq <= EPSILON)
    {
        return true;
    }

    float distance = sqrt(distanceSq);
    float3 lightDir = toLight / distance;
    float NdotL = max(dot(shadingNormal, lightDir), 0.0f);
    if (NdotL <= 0.0f)
    {
        return true;
    }

    float rawLightCos = dot(lightNormal, -lightDir);
    float lightCos = (0 != (lightMat.materialFlags & MATERIAL_FLAG_DOUBLE_SIDED)) ? abs(rawLightCos) : max(rawLightCos, 0.0f);
    if (lightCos <= 0.0f)
    {
        return true;
    }

    float3 bsdfCos = RestirEvalBSDF(surface, viewDir, lightDir, RESTIR_BSDF_LOBE_ALL);
    float bsdfPdf = RestirEvalPdfBSDF(surface, viewDir, lightDir, RESTIR_BSDF_LOBE_ALL);
    sourceBsdfPdf = bsdfPdf;
    sourceGeometry = lightCos / distanceSq;
    if (max(max(bsdfCos.x, bsdfCos.y), bsdfCos.z) <= 0.0f)
    {
        return true;
    }

    float entryPdf = selectedWeight / max(weightSum, EPSILON);
    float areaPdf = entryPdf / max((float)light.triangleCount * area, EPSILON);
    float neePdfSolidAngle = areaPdf * distanceSq / max(lightCos, EPSILON);
    lightPdf = neePdfSolidAngle;
    float misWeight = neePdfSolidAngle / max(neePdfSolidAngle + bsdfPdf, EPSILON);

    float3 shadowOrigin = hitPos + geometricNormal * 0.01f;
    float visibility = ShadowVisibility(g_scene, shadowOrigin, lightDir, RAY_TMIN, max(distance - 0.02f, RAY_TMIN), 0xFF);

    rcIrradiance = Le * visibility;
    contribution = bsdfCos * Le * lightCos * visibility * misWeight / max(distanceSq * areaPdf, EPSILON);
    return true;
}

void RestirUpdateReservoirFromPayload(inout RestirReservoir reservoir, RayPayload payload, inout uint rngSeed)
{
    if (0u == g_restirCandidateEnabled)
    {
        return;
    }

    bool hasPrimaryHit = 0u != (payload.primaryHitFlags & RESTIR_PRIMARY_HIT_VALID);
    bool isSkyEscape = 0u != (payload.pathFlags & RESTIR_PATH_FLAG_SKY_ESCAPE);
    if (!hasPrimaryHit && !isSkyEscape)
    {
        return;
    }

    if (0u != (payload.primaryHitFlags & RESTIR_PRIMARY_HIT_NEE_CANDIDATE) &&
        0u == (payload.pathFlags & RESTIR_PATH_FLAG_NEE))
    {
        return;
    }

    RestirPathSample candidate = RestirMakeCandidateFromPayload(payload);
    float resamplingWeight = RestirComputeResamplingWeight(RestirGetWeightTerms(candidate));
    RestirUpdateReservoir(reservoir, candidate, resamplingWeight, RandomValue(rngSeed));
}

[shader("raygeneration")]
void RayGenMain()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;
    uint pixelIndex = pixelCoord.y * screenSize.x + pixelCoord.x;

    uint rngSeed = pixelIndex * 9781u ^ pixelCoord.y * 6271u ^ screenSize.x * 7919u ^ (g_frameSeed + 1u) * 104729u ^ 124623u;

    float3 finalColor = 0.0f.xxx;
    RestirReservoir restirReservoir = RestirMakeEmptyReservoir();
    uint primaryHitFlags = 0u;

    for (uint bounce = 0; bounce < SPP; ++bounce)
    {
        float2 ndc = (float2(pixelCoord) + g_cameraJitterPixels + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
        ndc.y = -ndc.y;
        float4 nearPos = mul(float4(ndc, 0.0f, 1.0f), g_viewProjInverse);
        nearPos /= nearPos.w;
        float4 farPos = mul(float4(ndc, 1.0f, 1.0f), g_viewProjInverse);
        farPos /= farPos.w;

        RayDesc ray;
        ray.Origin = nearPos.xyz;
        ray.Direction = normalize(farPos.xyz - nearPos.xyz);
        ray.TMin = 0.001f;
        ray.TMax = RAY_TMAX;

        RayPayload payload = MakeDefaultRayPayload<RayPayload>(0);
        payload.pixelIndex = pixelIndex;

        TraceRay(g_scene,
                 RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
                 0xFF,
                 0, 0, 0,
                 ray,
                 payload);
        finalColor += payload.color.rgb;
        primaryHitFlags |= payload.primaryHitFlags & RESTIR_PRIMARY_HIT_VALID;

        RestirUpdateReservoirFromPayload(restirReservoir, payload, rngSeed);

        if (0u != g_restirCandidateEnabled && 0u != (payload.primaryHitFlags & RESTIR_PRIMARY_HIT_VALID))
        {
            RayPayload sunPayload = MakeDefaultRayPayload<RayPayload>(0);
            sunPayload.pixelIndex = pixelIndex;
            sunPayload.primaryHitFlags = RESTIR_PRIMARY_HIT_NEE_CANDIDATE | RESTIR_PRIMARY_HIT_NEE_SUN;

            TraceRay(g_scene,
                     RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
                     0xFF,
                     0, 0, 0,
                     ray,
                     sunPayload);

            RestirUpdateReservoirFromPayload(restirReservoir, sunPayload, rngSeed);

            if (g_restirEmissiveLightCount > 0u)
            {
                RayPayload emissivePayload = MakeDefaultRayPayload<RayPayload>(0);
                emissivePayload.pixelIndex = pixelIndex;
                emissivePayload.primaryHitFlags =
                    RESTIR_PRIMARY_HIT_NEE_CANDIDATE | RESTIR_PRIMARY_HIT_NEE_EMISSIVE;

                TraceRay(g_scene,
                         RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
                         0xFF,
                         0, 0, 0,
                         ray,
                         emissivePayload);

                RestirUpdateReservoirFromPayload(restirReservoir, emissivePayload, rngSeed);
            }
        }
    }
    finalColor /= SPP;

    if (0u != g_restirCandidateEnabled)
    {
        g_restirReservoirInitial[pixelIndex] = restirReservoir;
        if (0u == (primaryHitFlags & RESTIR_PRIMARY_HIT_VALID))
        {
            RestirWriteInvalidPrimaryOutputs(pixelIndex);
        }

        return;
    }

    float3 outputColor = finalColor;
    if (PT_DEBUG_VIEW == 0 && !UsePhysicalRenderingMode())
    {
        outputColor *= PT_OUTPUT_EXPOSURE;
        float3 bloom = max(0.0f.xxx, outputColor - PT_BLOOM_THRESHOLD.xxx) * PT_BLOOM_INTENSITY;
        outputColor += bloom;
    }

    g_output[pixelCoord] = float4(max(0.0f.xxx, outputColor), 1.0f);
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    float3 environment = SampleSkyEnvironment(WorldRayDirection(), g_environmentMode);
    payload.color = environment;
    payload.pathFlags = RESTIR_PATH_FLAG_SKY_ESCAPE | RESTIR_METADATA_SHIFT_DATA_VALID;
    payload.packedRcVertexWi = RestirPackNormalOct16(normalize(WorldRayDirection()));
    RestirSetPayloadRcIrradiance(payload, environment);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    InstanceData inst = g_instanceBuffer[InstanceID()];
    GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + GeometryIndex()];
    MaterialGPUData mat = g_materials[geo.materialIdx];

    StructuredBuffer<Vertex> vBuffer = ResourceDescriptorHeap[inst.vertexBufferIdx];
    Buffer<uint> iBuffer = ResourceDescriptorHeap[inst.indexBufferIdx];

    float3 bary;
    bary.x = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
    bary.y = attribs.barycentrics.x;
    bary.z = attribs.barycentrics.y;

    uint triIdx = PrimitiveIndex();
    uint i0 = iBuffer[geo.indexBase + triIdx * 3 + 0];
    uint i1 = iBuffer[geo.indexBase + triIdx * 3 + 1];
    uint i2 = iBuffer[geo.indexBase + triIdx * 3 + 2];

    Vertex v0 = vBuffer[i0];
    Vertex v1 = vBuffer[i1];
    Vertex v2 = vBuffer[i2];

    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float visibleNormalStrength = UsePhysicalRenderingMode() ? 1.0f : PT_VISIBLE_NORMAL_STRENGTH;
    RestirSurface surface;
    RestirBuildSurface(inst, mat, v0, v1, v2, bary, hitPos, visibleNormalStrength, surface);

    float3 geometricNormal = surface.geometricNormal;
    float3 normal = surface.normal;
    float3 visibleNormal = surface.shadingNormal;
    if (dot(normal, WorldRayDirection()) > 0.0f)
    {
        normal = -normal;
    }
    if (dot(visibleNormal, WorldRayDirection()) > 0.0f)
    {
        visibleNormal = -visibleNormal;
    }
    surface.normal = normal;
    surface.shadingNormal = visibleNormal;

    float2 uv = surface.uv;
    float3 albedo = surface.albedo;
    float metallic = surface.metallic;
    float roughness = surface.roughness;
    float ao = surface.ao;

    if (payload.recursionDepth == 0)
    {
        payload.primaryHitFlags =
            (payload.primaryHitFlags &
             (RESTIR_PRIMARY_HIT_NEE_CANDIDATE | RESTIR_PRIMARY_HIT_NEE_SUN | RESTIR_PRIMARY_HIT_NEE_EMISSIVE)) |
            RESTIR_PRIMARY_HIT_VALID;
        if (0u != g_restirCandidateEnabled)
        {
            RestirWritePrimaryOutputs(
                payload.pixelIndex,
                hitPos,
                RayTCurrent(),
                geometricNormal,
                visibleNormal,
                albedo,
                metallic,
                roughness,
                inst,
                mat,
                geo,
                GeometryIndex(),
                PrimitiveIndex(),
                attribs.barycentrics);
        }
    }
    else if (payload.recursionDepth == 1)
    {
        payload.reconnectInstanceId = inst.instanceID;
        payload.reconnectInstanceGeneration = inst.generation;
        payload.reconnectGeometryIndex = GeometryIndex();
        payload.reconnectPrimitiveIndex = PrimitiveIndex();
        payload.reconnectBarycentrics = RestirPackBarycentrics(attribs.barycentrics);
        float reconnectDistanceSq = max(RayTCurrent() * RayTCurrent(), EPSILON);
        float reconnectCos = abs(dot(surface.faceNormal, -normalize(WorldRayDirection())));
        payload.rcSourceGeometry = reconnectCos / reconnectDistanceSq;
        payload.pathFlags |= RESTIR_METADATA_SHIFT_DATA_VALID;
    }

    float3 V = -normalize(WorldRayDirection());
    float NdotVGeom = max(dot(geometricNormal, V), 0.0f);
    float NdotVShading = max(dot(visibleNormal, V), 0.0f);
    float3 F0 = lerp(0.04f.xxx, albedo, metallic);
    float3 emissive = EvaluateMaterialEmission(mat, uv, g_sampler);
    uint rngSeed =
        InstanceID() * 0xc2b2ae35u ^
        PrimitiveIndex() * 0x85ebca6bu ^
        payload.recursionDepth * 0x27d4eb2du ^
        (g_frameSeed + 1u) * 0x165667b1u ^
        (asuint(hitPos.x) + asuint(hitPos.y) * 0x9e3779b9u + asuint(hitPos.z));
    rngSeed ^= rngSeed >> 16;
    rngSeed *= 0x7feb352d;
    rngSeed ^= rngSeed >> 15;
    rngSeed *= 0x846ca68b;
    rngSeed ^= rngSeed >> 16;

    if (payload.recursionDepth == 0 && PT_DEBUG_VIEW != 0)
    {
        if (PT_DEBUG_VIEW == 1)
        {
            payload.color = visibleNormal * 0.5f + 0.5f;
        }
        else if (PT_DEBUG_VIEW == 2)
        {
            payload.color = geometricNormal * 0.5f + 0.5f;
        }
        else if (PT_DEBUG_VIEW == 3)
        {
            payload.color = float3(NdotVShading, NdotVGeom, 0.0f);
        }
        else if (PT_DEBUG_VIEW == 4)
        {
            payload.color = albedo;
        }
        else
        {
            payload.color = float3(metallic, roughness, ao);
        }
        return;
    }

    if (payload.recursionDepth == 0 && 0u != (payload.primaryHitFlags & RESTIR_PRIMARY_HIT_NEE_CANDIDATE))
    {
        if (max(max(emissive.x, emissive.y), emissive.z) > 0.0f)
        {
            payload.color = 0.0f.xxx;
            return;
        }

        float3 directLighting = 0.0f.xxx;
        if (0u != (payload.primaryHitFlags & RESTIR_PRIMARY_HIT_NEE_SUN))
        {
            float sunLightPdf;
            float3 sampledSunDirection;
            float3 sampledSunIrradiance;
            EvaluateEnvironmentSunNEE(
                g_scene,
                g_environmentMode,
                hitPos,
                geometricNormal,
                visibleNormal,
                V,
                albedo,
                metallic,
                roughness,
                ao,
                F0,
                1u,
                sunLightPdf,
                sampledSunDirection,
                sampledSunIrradiance,
                rngSeed);
            directLighting = RestirEvalBSDF(
                surface,
                V,
                sampledSunDirection,
                RESTIR_BSDF_LOBE_ALL
            ) * sampledSunIrradiance / max(sunLightPdf, EPSILON);
            payload.lightPdf = sunLightPdf;
            if (sunLightPdf > 0.0f)
            {
                payload.pathFlags =
                    RESTIR_PATH_FLAG_NEE |
                    RESTIR_PATH_FLAG_NEE_SUN |
                    RESTIR_METADATA_SHIFT_DATA_VALID |
                    RESTIR_METADATA_RC_FINAL |
                    RESTIR_METADATA_RC_NEE_TERMINAL;
                RestirSetPayloadLobe(payload, true, RESTIR_BSDF_LOBE_ALL, false);
                payload.packedRcVertexWi = RestirPackNormalOct16(sampledSunDirection);
                RestirSetPayloadRcIrradiance(payload, sampledSunIrradiance);
                payload.rcSourcePdfBefore = RestirEvalPdfBSDF(
                    surface,
                    V,
                    sampledSunDirection,
                    RESTIR_BSDF_LOBE_ALL
                );
                payload.rcSourceGeometry = 1.0f;
            }
        }
        else if (0u != (payload.primaryHitFlags & RESTIR_PRIMARY_HIT_NEE_EMISSIVE))
        {
            uint lightInstanceId;
            uint lightInstanceGeneration;
            uint lightGeometryIndex;
            uint lightPrimitiveIndex;
            uint lightBarycentrics;
            float3 rcIrradiance;
            float sourceBsdfPdf;
            float sourceGeometry;
            bool sampledEmissive = EvaluateRestirEmissiveNEE(
                surface,
                V,
                rngSeed,
                directLighting,
                payload.lightPdf,
                lightInstanceId,
                lightInstanceGeneration,
                lightGeometryIndex,
                lightPrimitiveIndex,
                lightBarycentrics,
                rcIrradiance,
                sourceBsdfPdf,
                sourceGeometry);

            if (sampledEmissive)
            {
                payload.pathFlags =
                    RESTIR_PATH_FLAG_NEE |
                    RESTIR_PATH_FLAG_NEE_EMISSIVE |
                    RESTIR_METADATA_SHIFT_DATA_VALID |
                    RESTIR_METADATA_RC_FINAL |
                    RESTIR_METADATA_RC_NEE_TERMINAL;
                RestirSetPayloadLobe(payload, true, RESTIR_BSDF_LOBE_ALL, false);
                payload.reconnectInstanceId = lightInstanceId;
                payload.reconnectInstanceGeneration = lightInstanceGeneration;
                payload.reconnectGeometryIndex = lightGeometryIndex;
                payload.reconnectPrimitiveIndex = lightPrimitiveIndex;
                payload.reconnectBarycentrics = lightBarycentrics;
                payload.rcSourcePdfBefore = sourceBsdfPdf;
                payload.rcSourceGeometry = sourceGeometry;
                RestirSetPayloadRcIrradiance(payload, rcIrradiance);
            }
        }

        payload.color = directLighting;
        return;
    }

    if (max(max(emissive.x, emissive.y), emissive.z) > 0.0f)
    {
        float3 emissionContribution = (0 == payload.recursionDepth)
                                          ? EvaluateCameraVisibleMaterialEmission(mat, uv, g_emissionViewMode, g_sampler)
                                          : emissive;
        payload.color = emissionContribution;
        payload.lightPdf = RestirComputeEmissiveHitNeePdfSolidAngle(inst, geo, mat, v0, v1, v2, hitPos, WorldRayOrigin());
        payload.pathFlags =
            (payload.pathFlags & ~RESTIR_PATH_EVENT_MASK) |
            RESTIR_PATH_FLAG_EMISSIVE_HIT;
        if (payload.recursionDepth == 1)
        {
            payload.pathFlags |= RESTIR_METADATA_SHIFT_DATA_VALID | RESTIR_METADATA_RC_FINAL;
            RestirSetPayloadRcIrradiance(payload, emissive);
        }
        return;
    }

    if (payload.recursionDepth >= MAX_RECURSION_DEPTH)
    {
        payload.color = emissive;
        return;
    }

    float specProb = RestirComputeSpecularProbability(surface, V);

    float xi = RandomValue(rngSeed);
    bool chooseSpec = (xi < specProb);
    uint sampledLobe = chooseSpec ? RESTIR_BSDF_LOBE_SPECULAR : RESTIR_BSDF_LOBE_DIFFUSE;
    bool sampledDelta = chooseSpec && roughness < 0.15f;

    float3 L;
    float pdf;     // Sampling pdf (mixture)
    float3 weight; // 1-step throughput
    float3 contributionNumerator;

    if (!chooseSpec)
    {
        L = CosineHemisphere(visibleNormal, rngSeed);
        float NdotL = max(dot(visibleNormal, L), 0.0f);
        if (NdotL <= 0.0f)
        {
            payload.color = emissive;
            return;
        }

        contributionNumerator = RestirEvalBSDF(surface, V, L, RESTIR_BSDF_LOBE_DIFFUSE);
        pdf = max(RestirEvalPdfBSDF(surface, V, L, RESTIR_BSDF_LOBE_DIFFUSE), EPSILON);
        weight = contributionNumerator / pdf;
    }
    else
    {
        if (roughness < 0.15f)
        {
            L = reflect(WorldRayDirection(), visibleNormal);
            float NdotL = max(dot(visibleNormal, L), 0.0f);
            if (NdotL <= 0.0f)
            {
                payload.color = emissive;
                return;
            }

            contributionNumerator = RestirEvalBSDF(surface, V, L, RESTIR_BSDF_LOBE_SPECULAR);
            pdf = max(RestirEvalPdfBSDF(surface, V, L, RESTIR_BSDF_LOBE_SPECULAR), EPSILON);
            weight = contributionNumerator / pdf;
        }
        else
        {
            L = SampleGGX(visibleNormal, V, roughness, rngSeed);
            float NdotL = max(dot(visibleNormal, L), 0.0f);
            if (NdotL <= 0.0f)
            {
                payload.color = emissive;
                return;
            }

            contributionNumerator = RestirEvalBSDF(surface, V, L, RESTIR_BSDF_LOBE_SPECULAR);
            pdf = max(RestirEvalPdfBSDF(surface, V, L, RESTIR_BSDF_LOBE_SPECULAR), EPSILON);
            weight = contributionNumerator / pdf;
        }
    }

    float maxW = max(weight.x, max(weight.y, weight.z));
    if (maxW < 1e-5f)
    {
        payload.color = emissive;
        return;
    }

    RayDesc nextRay;
    nextRay.Origin = hitPos + geometricNormal * 0.01f;
    nextRay.Direction = normalize(L);
    nextRay.TMin = RAY_TMIN;
    nextRay.TMax = RAY_TMAX;

    RayPayload child = MakeDefaultRayPayload<RayPayload>(payload.recursionDepth + 1);

    TraceRay(
        g_scene,
        RAY_FLAG_FORCE_OPAQUE,
        0xFF,
        0,
        0,
        0,
        nextRay,
        child);

    if (payload.reconnectInstanceId == 0xffffffffu && child.reconnectInstanceId != 0xffffffffu)
    {
        payload.reconnectInstanceId = child.reconnectInstanceId;
        payload.reconnectInstanceGeneration = child.reconnectInstanceGeneration;
        payload.reconnectGeometryIndex = child.reconnectGeometryIndex;
        payload.reconnectPrimitiveIndex = child.reconnectPrimitiveIndex;
        payload.reconnectBarycentrics = child.reconnectBarycentrics;
    }

    if (payload.recursionDepth == 0)
    {
        payload.packedRcVertexWi = child.packedRcVertexWi;
        payload.packedRcVertexIrradianceXY = child.packedRcVertexIrradianceXY;
        payload.packedRcVertexIrradianceZ = child.packedRcVertexIrradianceZ;
        payload.rcSourcePdfBefore = pdf;
        payload.rcSourcePdfAfter = child.rcSourcePdfAfter;
        payload.rcSourceGeometry = child.rcSourceGeometry;
    }
    else if (payload.recursionDepth == 1)
    {
        payload.packedRcVertexWi = RestirPackNormalOct16(normalize(L));
        RestirSetPayloadRcIrradiance(payload, child.color);
        payload.rcSourcePdfAfter = pdf;
    }

    bool applyEmissiveHitMis =
        0u != (child.pathFlags & RESTIR_PATH_FLAG_EMISSIVE_HIT) &&
        0u == (child.pathFlags & RESTIR_PATH_FLAG_MIS_APPLIED) &&
        child.lightPdf > 0.0f;
    if (applyEmissiveHitMis)
    {
        float emissiveHitMisWeight = pdf / max(pdf + child.lightPdf, EPSILON);
        child.color *= emissiveHitMisWeight;
        child.pathFlags |= RESTIR_PATH_FLAG_MIS_APPLIED;
    }

    payload.color = emissive + weight * child.color;
    payload.lightPdf = child.lightPdf;
    payload.pathFlags = child.pathFlags;
    if (payload.recursionDepth == 0)
    {
        RestirSetPayloadLobe(payload, true, sampledLobe, sampledDelta);
    }
    else if (payload.recursionDepth == 1)
    {
        payload.pathFlags |= RESTIR_METADATA_SHIFT_DATA_VALID;
        if (applyEmissiveHitMis)
        {
            payload.pathFlags |= RESTIR_METADATA_RC_SUFFIX_EMISSIVE_MIS;
        }
        RestirSetPayloadLobe(payload, false, sampledLobe, sampledDelta);
    }
}
