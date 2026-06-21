#ifndef RESTIR_RESERVOIR_HLSLI
#define RESTIR_RESERVOIR_HLSLI

#include "RaytracingCommon.h"

float RestirLuminance(float3 color)
{
    return dot(max(0.0f.xxx, color), float3(0.2126f, 0.7152f, 0.0722f));
}

float RestirTargetFromPathContribution(float3 pathContribution)
{
    return RestirLuminance(pathContribution);
}

struct RestirWeightTerms
{
    float target;
    float contributionWeight;
    float misWeight;
    float shiftJacobian;
};

RestirWeightTerms RestirMakeInitialCandidateWeightTerms(float3 targetContribution, float sourcePdf)
{
    RestirWeightTerms terms;
    terms.target = RestirTargetFromPathContribution(targetContribution);
    terms.contributionWeight = rcp(max(sourcePdf, EPSILON));
    terms.misWeight = 1.0f;
    terms.shiftJacobian = 1.0f;
    return terms;
}

float RestirComputeResamplingWeight(RestirWeightTerms terms)
{
    return terms.target * terms.contributionWeight * terms.misWeight * terms.shiftJacobian;
}

float2 RestirOctWrap(float2 v)
{
    float2 signNotZero = float2(v.x >= 0.0f ? 1.0f : -1.0f, v.y >= 0.0f ? 1.0f : -1.0f);
    return (1.0f - abs(v.yx)) * signNotZero;
}

uint RestirPackNormalOct16(float3 normal)
{
    float3 n = normalize(normal);
    n /= max(abs(n.x) + abs(n.y) + abs(n.z), EPSILON);

    float2 encoded = n.xy;
    if (n.z < 0.0f)
    {
        encoded = RestirOctWrap(encoded);
    }
    
    uint x = (uint) round(saturate(encoded.x * 0.5f + 0.5f) * 65535.0f);
    uint y = (uint) round(saturate(encoded.y * 0.5f + 0.5f) * 65535.0f);
    return x | (y << 16u);
}

float3 RestirUnpackNormalOct16(uint packed)
{
    float2 encoded = float2((float) (packed & 0xffffu), (float) ((packed >> 16u) & 0xffffu));
    encoded = encoded / 65535.0f * 2.0f - 1.0f;

    float3 n = float3(encoded.x, encoded.y, 1.0f - abs(encoded.x) - abs(encoded.y));
    if (n.z < 0.0f)
    {
        n.xy = RestirOctWrap(n.xy);
    }

    return normalize(n);
}

uint RestirPackUnorm16(float value)
{
    return (uint) round(saturate(value) * 65535.0f);
}

float RestirUnpackUnorm16(uint packed)
{
    return (float) (packed & 0xffffu) / 65535.0f;
}

uint RestirPackBarycentrics(float2 barycentrics)
{
    uint x = (uint) round(saturate(barycentrics.x) * 65535.0f);
    uint y = (uint) round(saturate(barycentrics.y) * 65535.0f);
    return x | (y << 16u);
}

float2 RestirUnpackBarycentrics(uint packed)
{
    return float2((float) (packed & 0xffffu), (float) ((packed >> 16u) & 0xffffu)) / 65535.0f;
}

RestirPrimaryHit RestirMakeInvalidPrimaryHit()
{
    RestirPrimaryHit hit;
    hit.positionDistance = float4(0.0f, 0.0f, 0.0f, -1.0f);
    hit.packedNormal = RestirPackNormalOct16(float3(0.0f, 1.0f, 0.0f));
    hit.packedRoughness = RestirPackUnorm16(1.0f);
    hit.instanceId = 0xffffffffu;
    hit.materialId = 0xffffffffu;
    hit.geometryId = 0xffffffffu;
    hit.flags = 0u;
    return hit;
}

RestirPathSample RestirMakeEmptyPathSample()
{
    RestirPathSample sample;
    sample.contributionTarget = 0.0f.xxxx;
    sample.throughputPdf = float4(0.0f, asfloat(0u), 1.0f, 1.0f);
    sample.weightTerms = float4(0.0f, 1.0f, 1.0f, 1.0f);
    sample.pathLength = 0u;
    sample.sourceKind = RESTIR_SOURCE_CURRENT_PIXEL_FRESH_PATH;
    sample.shiftKind = RESTIR_SHIFT_IDENTITY;
    sample.reconnectInstanceGeneration = 0u;
    sample.reconnectInstanceId = 0xffffffffu;
    sample.reconnectGeometryIndex = 0xffffffffu;
    sample.reconnectPrimitiveIndex = 0xffffffffu;
    sample.reconnectBarycentrics = 0u;
    return sample;
}

RestirReservoir RestirMakeEmptyReservoir()
{
    RestirReservoir reservoir;
    reservoir.sample = RestirMakeEmptyPathSample();
    reservoir.resamplingWeightSum = 0.0f;
    reservoir.selectedResamplingWeight = 0.0f;
    reservoir.sampleCount = 0u;
    reservoir.flags = 0u;
    return reservoir;
}

bool RestirHasReconnectHitRef(RestirPathSample sample)
{
    return sample.reconnectInstanceId != 0xffffffffu &&
           sample.reconnectGeometryIndex != 0xffffffffu &&
           sample.reconnectPrimitiveIndex != 0xffffffffu;
}

bool RestirIsSkyEscapeSample(RestirPathSample sample)
{
    return 0u != (asuint(sample.throughputPdf.y) & RESTIR_PATH_FLAG_SKY_ESCAPE);
}

float RestirGetLightPdf(RestirPathSample sample)
{
    return sample.throughputPdf.x;
}

uint RestirGetPathFlags(RestirPathSample sample)
{
    return asuint(sample.throughputPdf.y);
}

void RestirSetLightPdfAndPathFlags(inout RestirPathSample sample, float lightPdf, uint pathFlags)
{
    sample.throughputPdf.x = lightPdf;
    sample.throughputPdf.y = asfloat(pathFlags);
}

#ifdef RESTIR_ENABLE_PAYLOAD_HELPERS
RestirPathSample RestirMakeCandidateFromPayload(RayPayload payload)
{
    RestirPathSample sample = RestirMakeEmptyPathSample();
    float3 targetContribution = max(0.0f.xxx, payload.targetContribution);
    float sourcePdf = max(payload.sourcePdf, EPSILON);
    RestirWeightTerms terms = RestirMakeInitialCandidateWeightTerms(targetContribution, sourcePdf);
    sample.contributionTarget = float4(targetContribution, terms.target);
    sample.throughputPdf = float4(payload.lightPdf, asfloat(payload.pathFlags), 1.0f, sourcePdf);
    sample.weightTerms = float4(terms.target, terms.contributionWeight, terms.misWeight, terms.shiftJacobian);
    sample.pathLength = payload.recursionDepth + 1u;
    sample.sourceKind = RESTIR_SOURCE_CURRENT_PIXEL_FRESH_PATH;
    sample.shiftKind = RESTIR_SHIFT_IDENTITY;
    sample.reconnectInstanceId = payload.reconnectInstanceId;
    sample.reconnectInstanceGeneration = payload.reconnectInstanceGeneration;
    sample.reconnectGeometryIndex = payload.reconnectGeometryIndex;
    sample.reconnectPrimitiveIndex = payload.reconnectPrimitiveIndex;
    sample.reconnectBarycentrics = payload.reconnectBarycentrics;
    return sample;
}
#endif

RestirWeightTerms RestirGetWeightTerms(RestirPathSample sample)
{
    RestirWeightTerms terms;
    terms.target = sample.weightTerms.x;
    terms.contributionWeight = sample.weightTerms.y;
    terms.misWeight = sample.weightTerms.z;
    terms.shiftJacobian = sample.weightTerms.w;
    return terms;
}

void RestirUpdateReservoir(inout RestirReservoir reservoir, RestirPathSample candidate, float resamplingWeight, float randomValue)
{
    ++reservoir.sampleCount;

    if (resamplingWeight <= 0.0f)
    {
        return;
    }

    reservoir.resamplingWeightSum += resamplingWeight;

    float selectionProbability = resamplingWeight / max(reservoir.resamplingWeightSum, EPSILON);
    if (randomValue <= selectionProbability)
    {
        reservoir.sample = candidate;
        reservoir.selectedResamplingWeight = resamplingWeight;
        reservoir.flags = RESTIR_RESERVOIR_VALID;
    }
}

bool RestirIsValidReservoir(RestirReservoir reservoir)
{
    return 0u != (reservoir.flags & RESTIR_RESERVOIR_VALID);
}

bool RestirIsSkyEscapeReservoir(RestirReservoir reservoir)
{
    return RestirIsValidReservoir(reservoir) && RestirIsSkyEscapeSample(reservoir.sample);
}

bool RestirIsValidPrimaryHit(RestirPrimaryHit hit)
{
    return 0u != (hit.flags & RESTIR_PRIMARY_HIT_VALID);
}

// Temporary convention: sourceKind currently selects the final normalization path
// for temporal GRIS output. Move this to reservoir-level flags with the v2 layout.
bool RestirUsesGrisFinalize(RestirReservoir reservoir)
{
    return reservoir.sample.sourceKind == RESTIR_SOURCE_TEMPORAL_REUSE;
}

float RestirContributionWeightFromReservoir(RestirReservoir reservoir)
{
    if (!RestirIsValidReservoir(reservoir))
    {
        return 0.0f;
    }

    float target = max(reservoir.sample.weightTerms.x, EPSILON);
    if (RestirUsesGrisFinalize(reservoir))
    {
        return reservoir.resamplingWeightSum / target;
    }

    return reservoir.resamplingWeightSum / (target * max(1.0f, float(reservoir.sampleCount)));
}

float3 RestirDebugColor(RestirReservoir reservoir, RestirPrimaryHit currentHit, uint debugView)
{
    bool validReservoir = 0u != (reservoir.flags & RESTIR_RESERVOIR_VALID);
    bool validCurrentHit = 0u != (currentHit.flags & RESTIR_PRIMARY_HIT_VALID);
    float3 invalidColor = float3(0.35f, 0.0f, 0.0f);

    if (1u == debugView)
    {
        return validReservoir ? reservoir.sample.contributionTarget.rgb : invalidColor;
    }
    if (2u == debugView)
    {
        return validReservoir ? float3(0.1f, 1.0f, 0.2f) : invalidColor;
    }
    if (3u == debugView)
    {
        if (!validReservoir)
        {
            return invalidColor;
        }
        float heat = saturate(log2(1.0f + reservoir.resamplingWeightSum) / 8.0f);
        return float3(heat, heat * heat, 1.0f - heat);
    }
    if (4u == debugView)
    {
        return validCurrentHit ? RestirUnpackNormalOct16(currentHit.packedNormal) * 0.5f + 0.5f : invalidColor;
    }
    if (5u == debugView)
    {
        return validCurrentHit ? RestirUnpackNormalOct16(currentHit.packedNormal) * 0.5f + 0.5f : invalidColor;
    }
    if (6u == debugView)
    {
        if (!validReservoir || !validCurrentHit)
        {
            return invalidColor;
        }
        float roughness = RestirUnpackUnorm16(currentHit.packedRoughness);
        return float3(roughness, reservoir.sampleCount > 0u ? 1.0f : 0.0f, 0.0f);
    }
    if (7u == debugView)
    {
        if (!validCurrentHit)
        {
            return invalidColor;
        }
        float roughness = RestirUnpackUnorm16(currentHit.packedRoughness);
        return float3(roughness, 1.0f, 0.0f);
    }

    return 0.0f.xxx;
}

#endif // RESTIR_RESERVOIR_HLSLI
