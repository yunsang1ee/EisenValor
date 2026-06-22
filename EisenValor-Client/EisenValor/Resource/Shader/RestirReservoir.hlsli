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

RestirWeightTerms RestirMakeInitialCandidateWeightTerms(float3 targetContribution)
{
    RestirWeightTerms terms;
    terms.target = RestirTargetFromPathContribution(targetContribution);
    terms.contributionWeight = 1.0f;
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

void RestirPackRadianceHalf3(float3 radiance, out uint packedXY, out uint packedZ)
{
    float3 finiteRadiance = min(max(radiance, 0.0f.xxx), 65504.0f.xxx);
    packedXY = f32tof16(finiteRadiance.x) | (f32tof16(finiteRadiance.y) << 16u);
    packedZ = f32tof16(finiteRadiance.z);
}

float3 RestirUnpackRadianceHalf3(uint packedXY, uint packedZ)
{
    return float3(
        f16tof32(packedXY & 0xffffu),
        f16tof32(packedXY >> 16u),
        f16tof32(packedZ & 0xffffu)
    );
}

uint RestirPackUnorm16(float value)
{
    return (uint) round(saturate(value) * 65535.0f);
}

float RestirUnpackUnorm16(uint packed)
{
    return (float) (packed & 0xffffu) / 65535.0f;
}

uint RestirPackPrimaryHitRoughnessFlags(float roughness, uint flags)
{
    return RestirPackUnorm16(roughness) | ((flags & 0xffffu) << 16u);
}

float RestirGetPrimaryHitRoughness(RestirPrimaryHit hit)
{
    return RestirUnpackUnorm16(hit.packedRoughnessFlags);
}

uint RestirGetPrimaryHitFlags(RestirPrimaryHit hit)
{
    return hit.packedRoughnessFlags >> 16u;
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

uint RestirSetMetadataLobe(uint metadata, uint shift, uint lobeFlags)
{
    uint mask = RESTIR_METADATA_LOBE_MASK << shift;
    return (metadata & ~mask) | ((lobeFlags & RESTIR_METADATA_LOBE_MASK) << shift);
}

uint RestirGetMetadataLobe(uint metadata, uint shift)
{
    return (metadata >> shift) & RESTIR_METADATA_LOBE_MASK;
}

uint RestirSetMetadataSourceKind(uint metadata, uint sourceKind)
{
    return (metadata & ~RESTIR_METADATA_SOURCE_KIND_MASK) |
           ((sourceKind << RESTIR_METADATA_SOURCE_KIND_SHIFT) & RESTIR_METADATA_SOURCE_KIND_MASK);
}

uint RestirSetMetadataShiftKind(uint metadata, uint shiftKind)
{
    return (metadata & ~RESTIR_METADATA_SHIFT_KIND_MASK) |
           ((shiftKind << RESTIR_METADATA_SHIFT_KIND_SHIFT) & RESTIR_METADATA_SHIFT_KIND_MASK);
}

uint RestirGetSampleMetadata(RestirPathSample sample)
{
    return asuint(sample.throughputPdf.y);
}

void RestirSetSampleMetadata(inout RestirPathSample sample, uint metadata)
{
    sample.throughputPdf.y = asfloat(metadata);
}

uint RestirGetSampleSourceKind(RestirPathSample sample)
{
    return (RestirGetSampleMetadata(sample) & RESTIR_METADATA_SOURCE_KIND_MASK) >>
           RESTIR_METADATA_SOURCE_KIND_SHIFT;
}

void RestirSetSampleSourceKind(inout RestirPathSample sample, uint sourceKind)
{
    RestirSetSampleMetadata(sample, RestirSetMetadataSourceKind(RestirGetSampleMetadata(sample), sourceKind));
}

uint RestirGetSampleShiftKind(RestirPathSample sample)
{
    return (RestirGetSampleMetadata(sample) & RESTIR_METADATA_SHIFT_KIND_MASK) >>
           RESTIR_METADATA_SHIFT_KIND_SHIFT;
}

void RestirSetSampleShiftKind(inout RestirPathSample sample, uint shiftKind)
{
    RestirSetSampleMetadata(sample, RestirSetMetadataShiftKind(RestirGetSampleMetadata(sample), shiftKind));
}

uint RestirGetLobeBeforeRc(RestirPathSample sample)
{
    return RestirGetMetadataLobe(RestirGetSampleMetadata(sample), RESTIR_METADATA_LOBE_BEFORE_SHIFT);
}

uint RestirGetLobeAfterRc(RestirPathSample sample)
{
    return RestirGetMetadataLobe(RestirGetSampleMetadata(sample), RESTIR_METADATA_LOBE_AFTER_SHIFT);
}

bool RestirHasShiftData(RestirPathSample sample)
{
    return 0u != (RestirGetSampleMetadata(sample) & RESTIR_METADATA_SHIFT_DATA_VALID);
}

bool RestirIsDeltaBeforeRc(RestirPathSample sample)
{
    return 0u != (RestirGetSampleMetadata(sample) & RESTIR_METADATA_DELTA_BEFORE_RC);
}

bool RestirIsDeltaAfterRc(RestirPathSample sample)
{
    return 0u != (RestirGetSampleMetadata(sample) & RESTIR_METADATA_DELTA_AFTER_RC);
}

bool RestirIsRcFinal(RestirPathSample sample)
{
    return 0u != (RestirGetSampleMetadata(sample) & RESTIR_METADATA_RC_FINAL);
}

bool RestirIsRcNeeTerminal(RestirPathSample sample)
{
    return 0u != (RestirGetSampleMetadata(sample) & RESTIR_METADATA_RC_NEE_TERMINAL);
}

bool RestirRcSuffixNeedsEmissiveMis(RestirPathSample sample)
{
    return 0u != (RestirGetSampleMetadata(sample) & RESTIR_METADATA_RC_SUFFIX_EMISSIVE_MIS);
}

float3 RestirGetRcVertexWi(RestirPathSample sample)
{
    return RestirUnpackNormalOct16(sample.packedRcVertexWi);
}

float3 RestirGetRcVertexIrradiance(RestirPathSample sample)
{
    return RestirUnpackRadianceHalf3(
        sample.packedRcVertexIrradianceXY,
        sample.packedRcVertexIrradianceZ
    );
}

float RestirGetSourcePdfBeforeRc(RestirPathSample sample)
{
    return sample.throughputPdf.z;
}

float RestirGetSourcePdfAfterRc(RestirPathSample sample)
{
    return sample.throughputPdf.w;
}

float RestirGetSourceGeometry(RestirPathSample sample)
{
    return sample.contributionTarget.w;
}

RestirPrimaryHit RestirMakeInvalidPrimaryHit()
{
    RestirPrimaryHit hit;
    hit.positionDistance = float4(0.0f, 0.0f, 0.0f, -1.0f);
    hit.packedNormal = RestirPackNormalOct16(float3(0.0f, 1.0f, 0.0f));
    hit.packedRoughnessFlags = RestirPackPrimaryHitRoughnessFlags(1.0f, 0u);
    hit.instanceId = 0xffffffffu;
    hit.materialId = 0xffffffffu;
    hit.geometryId = 0xffffffffu;
    hit.geometryIndex = 0xffffffffu;
    hit.primitiveIndex = 0xffffffffu;
    hit.barycentrics = 0u;
    return hit;
}

RestirPathSample RestirMakeEmptyPathSample()
{
    RestirPathSample sample;
    sample.contributionTarget = 0.0f.xxxx;
    uint metadata = RestirSetMetadataSourceKind(0u, RESTIR_SOURCE_CURRENT_PIXEL_FRESH_PATH);
    metadata = RestirSetMetadataShiftKind(metadata, RESTIR_SHIFT_IDENTITY);
    sample.throughputPdf = float4(0.0f, asfloat(metadata), 0.0f, 0.0f);
    sample.weightTerms = float4(0.0f, 1.0f, 1.0f, 1.0f);
    sample.packedRcVertexWi = 0u;
    sample.packedRcVertexIrradianceXY = 0u;
    sample.packedRcVertexIrradianceZ = 0u;
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
    return RestirGetSampleMetadata(sample) & RESTIR_PATH_EVENT_MASK;
}

void RestirSetLightPdfAndPathFlags(inout RestirPathSample sample, float lightPdf, uint pathFlags)
{
    sample.throughputPdf.x = lightPdf;
    uint metadata = RestirGetSampleMetadata(sample);
    RestirSetSampleMetadata(sample, (metadata & ~RESTIR_PATH_EVENT_MASK) | (pathFlags & RESTIR_PATH_EVENT_MASK));
}

#ifdef RESTIR_ENABLE_PAYLOAD_HELPERS
RestirPathSample RestirMakeCandidateFromPayload(RayPayload payload)
{
    RestirPathSample sample = RestirMakeEmptyPathSample();
    float3 targetContribution = max(0.0f.xxx, payload.color);
    RestirWeightTerms terms = RestirMakeInitialCandidateWeightTerms(targetContribution);
    uint metadata = RestirSetMetadataSourceKind(payload.pathFlags, RESTIR_SOURCE_CURRENT_PIXEL_FRESH_PATH);
    metadata = RestirSetMetadataShiftKind(metadata, RESTIR_SHIFT_IDENTITY);
    sample.contributionTarget = float4(targetContribution, max(payload.rcSourceGeometry, 0.0f));
    sample.throughputPdf = float4(
        payload.lightPdf,
        asfloat(metadata),
        max(payload.rcSourcePdfBefore, 0.0f),
        max(payload.rcSourcePdfAfter, 0.0f)
    );
    sample.weightTerms = float4(terms.target, terms.contributionWeight, terms.misWeight, terms.shiftJacobian);
    sample.packedRcVertexWi = payload.packedRcVertexWi;
    sample.packedRcVertexIrradianceXY = payload.packedRcVertexIrradianceXY;
    sample.packedRcVertexIrradianceZ = payload.packedRcVertexIrradianceZ;
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

void RestirUpdateReservoir(
    inout RestirReservoir reservoir,
    RestirPathSample candidate,
    float resamplingWeight,
    float randomValue)
{
    ++reservoir.sampleCount;

    static const float maxFinite = 3.402823466e+38f;
    if (!(resamplingWeight > 0.0f && resamplingWeight < maxFinite))
    {
        return;
    }

    float updatedWeightSum = reservoir.resamplingWeightSum + resamplingWeight;
    if (!(updatedWeightSum > 0.0f && updatedWeightSum < maxFinite))
    {
        return;
    }
    reservoir.resamplingWeightSum = updatedWeightSum;

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
    return 0u != (RestirGetPrimaryHitFlags(hit) & RESTIR_PRIMARY_HIT_VALID);
}

// Temporal candidates use GRIS finalization; fresh candidates use identical-domain 1/M finalization.
bool RestirUsesGrisFinalize(RestirReservoir reservoir)
{
    return RestirGetSampleSourceKind(reservoir.sample) == RESTIR_SOURCE_TEMPORAL_REUSE;
}

float RestirContributionWeightFromReservoir(RestirReservoir reservoir)
{
    if (!RestirIsValidReservoir(reservoir))
    {
        return 0.0f;
    }

    float target = max(reservoir.sample.weightTerms.x, EPSILON);
    float contributionWeight;
    if (RestirUsesGrisFinalize(reservoir))
    {
        contributionWeight = reservoir.resamplingWeightSum / target;
    }
    else
    {
        contributionWeight =
            reservoir.resamplingWeightSum / (target * max(1.0f, float(reservoir.sampleCount)));
    }

    static const float maxFinite = 3.402823466e+38f;
    return contributionWeight >= 0.0f && contributionWeight < maxFinite
               ? contributionWeight
               : 0.0f;
}

float3 RestirDebugColor(RestirReservoir reservoir, RestirPrimaryHit currentHit, uint debugView)
{
    bool validReservoir = 0u != (reservoir.flags & RESTIR_RESERVOIR_VALID);
    bool validCurrentHit = RestirIsValidPrimaryHit(currentHit);
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
        float roughness = RestirGetPrimaryHitRoughness(currentHit);
        return float3(roughness, reservoir.sampleCount > 0u ? 1.0f : 0.0f, 0.0f);
    }
    if (7u == debugView)
    {
        if (!validCurrentHit)
        {
            return invalidColor;
        }
        float roughness = RestirGetPrimaryHitRoughness(currentHit);
        return float3(roughness, 1.0f, 0.0f);
    }

    if (8u == debugView)
    {
        if (!validReservoir)
        {
            return invalidColor;
        }
        float m = saturate(float(reservoir.sampleCount) / 21.0f);
        return float3(m, m, m);
    }

    return 0.0f.xxx;
}

#endif // RESTIR_RESERVOIR_HLSLI
