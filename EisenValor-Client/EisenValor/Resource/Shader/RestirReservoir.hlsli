#ifndef RESTIR_RESERVOIR_HLSLI
#define RESTIR_RESERVOIR_HLSLI

float RestirLuminance(float3 color)
{
    return dot(max(0.0f.xxx, color), float3(0.2126f, 0.7152f, 0.0722f));
}

float RestirTargetFromContribution(float3 contribution)
{
    return RestirLuminance(contribution);
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
    sample.throughputPdf = float4(1.0f, 1.0f, 1.0f, 1.0f);
    sample.firstHitPositionDistance = float4(0.0f, 0.0f, 0.0f, -1.0f);
    sample.packedFirstHitNormal = RestirPackNormalOct16(float3(0.0f, 1.0f, 0.0f));
    sample.packedFirstHitRoughness = RestirPackUnorm16(1.0f);
    sample.instanceId = 0xffffffffu;
    sample.materialId = 0xffffffffu;
    sample.geometryId = 0xffffffffu;
    sample.pathLength = 0u;
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

RestirPathSample RestirMakeCandidateFromPayload(RayPayload payload)
{
    RestirPathSample sample = RestirMakeEmptyPathSample();
    float3 contribution = max(0.0f.xxx, payload.color);
    float target = RestirTargetFromContribution(contribution);
    sample.contributionTarget = float4(contribution, target);
    sample.firstHitPositionDistance = float4(payload.primaryHitPosition, payload.primaryHitDistance);
    sample.packedFirstHitNormal = RestirPackNormalOct16(payload.primaryHitNormal);
    sample.packedFirstHitRoughness = RestirPackUnorm16(payload.roughness);
    sample.instanceId = payload.instanceId;
    sample.materialId = payload.materialId;
    sample.geometryId = payload.geometryId;
    sample.pathLength = payload.recursionDepth + 1u;
    return sample;
}

RestirPrimaryHit RestirPrimaryHitFromSample(RestirPathSample sample, uint reservoirFlags)
{
    if (0u == (reservoirFlags & RESTIR_RESERVOIR_VALID))
    {
        return RestirMakeInvalidPrimaryHit();
    }

    RestirPrimaryHit hit;
    hit.positionDistance = sample.firstHitPositionDistance;
    hit.packedNormal = sample.packedFirstHitNormal;
    hit.packedRoughness = sample.packedFirstHitRoughness;
    hit.instanceId = sample.instanceId;
    hit.materialId = sample.materialId;
    hit.geometryId = sample.geometryId;
    hit.flags = RESTIR_PRIMARY_HIT_VALID;
    return hit;
}

void RestirUpdateReservoir(inout RestirReservoir reservoir, RestirPathSample candidate, float resamplingWeight, float randomValue)
{
    if (resamplingWeight <= 0.0f)
    {
        return;
    }

    reservoir.resamplingWeightSum += resamplingWeight;
    ++reservoir.sampleCount;

    float selectionProbability = resamplingWeight / max(reservoir.resamplingWeightSum, EPSILON);
    if (randomValue <= selectionProbability)
    {
        reservoir.sample = candidate;
        reservoir.selectedResamplingWeight = resamplingWeight;
        reservoir.flags = RESTIR_RESERVOIR_VALID;
    }
}

float3 RestirDebugColor(RestirReservoir reservoir, RestirPrimaryHit hit, uint debugView)
{
    if (1u == debugView)
    {
        return (0u != (reservoir.flags & RESTIR_RESERVOIR_VALID)) ? float3(0.1f, 1.0f, 0.2f) : float3(0.35f, 0.0f, 0.0f);
    }
    if (2u == debugView)
    {
        float heat = saturate(log2(1.0f + reservoir.resamplingWeightSum) / 8.0f);
        return float3(heat, heat * heat, 1.0f - heat);
    }
    if (3u == debugView)
    {
        return (0u != (hit.flags & RESTIR_PRIMARY_HIT_VALID)) ? RestirUnpackNormalOct16(hit.packedNormal) * 0.5f + 0.5f : 0.0f.xxx;
    }
    if (4u == debugView)
    {
        float roughness = RestirUnpackUnorm16(hit.packedRoughness);
        return float3(roughness, reservoir.sampleCount > 0u ? 1.0f : 0.0f, 0.0f);
    }

    return reservoir.sample.contributionTarget.rgb;
}

#endif // RESTIR_RESERVOIR_HLSLI
