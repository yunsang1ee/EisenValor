#define HLSL
#include "RaytracingCommon.h"
#include "RestirReservoir.hlsli"

StructuredBuffer<RestirReservoir> g_restirReservoirInitial : register(t0, space0);
StructuredBuffer<RestirPrimaryHit> g_restirPrimaryHitCurrent : register(t1, space0);
StructuredBuffer<RestirReservoir> g_restirReservoirHistoryRead : register(t2, space0);
StructuredBuffer<RestirPrimaryHit> g_restirPrimaryHitHistoryRead : register(t3, space0);
Texture2D<float2> g_restirMotionVector : register(t4, space0);
RaytracingAccelerationStructure g_scene : register(t5, space0);
StructuredBuffer<InstanceData> g_instanceBuffer : register(t6, space0);
StructuredBuffer<GeoInfo> g_geoTable : register(t7, space0);
StructuredBuffer<uint> g_idToInstanceIndex : register(t8, space0);
RWStructuredBuffer<RestirReservoir> g_restirFinalReservoirWrite : register(u0, space0);
RWStructuredBuffer<RestirReservoir> g_restirReservoirHistoryWrite : register(u1, space0);
RWStructuredBuffer<RestirPrimaryHit> g_restirPrimaryHitHistoryWrite : register(u2, space0);

cbuffer RestirTemporalConstantsBuffer : register(b0, space0)
{
    RestirTemporalConstants g_restirTemporalConstants;
};

float RestirRandom(inout uint seed)
{
    seed = seed * 747796405u + 2891336453u;
    uint temp = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    temp = (temp >> 22u) ^ temp;
    return float(temp) / 4294967296.0f;
}

bool RestirIsFinitePositive(float value)
{
    return value > 0.0f && value < 3.402823466e+38f;
}

float3 RestirSafeNormalize(float3 value, float3 fallback)
{
    float lenSq = dot(value, value);
    return lenSq > EPSILON ? value * rsqrt(lenSq) : fallback;
}

bool RestirLoadReconnectSurface(RestirPathSample sample, out float3 position, out float3 normal)
{
    position = 0.0f.xxx;
    normal = float3(0.0f, 1.0f, 0.0f);

    if (!RestirHasReconnectHitRef(sample))
    {
        return false;
    }

    if (sample.reconnectInstanceId >= g_restirTemporalConstants.idToInstanceIndexCount)
    {
        return false;
    }
    uint instanceIndex = g_idToInstanceIndex[sample.reconnectInstanceId];
    if (instanceIndex >= g_restirTemporalConstants.instanceCount)
    {
        return false;
    }

    InstanceData inst = g_instanceBuffer[instanceIndex];
    if (inst.generation != sample.reconnectInstanceGeneration)
    {
        return false;
    }

    GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + sample.reconnectGeometryIndex];
    StructuredBuffer<Vertex> vBuffer = ResourceDescriptorHeap[inst.vertexBufferIdx];
    Buffer<uint> iBuffer = ResourceDescriptorHeap[inst.indexBufferIdx];

    uint triIdx = sample.reconnectPrimitiveIndex;
    uint i0 = iBuffer[geo.indexBase + triIdx * 3u + 0u];
    uint i1 = iBuffer[geo.indexBase + triIdx * 3u + 1u];
    uint i2 = iBuffer[geo.indexBase + triIdx * 3u + 2u];

    Vertex v0 = vBuffer[i0];
    Vertex v1 = vBuffer[i1];
    Vertex v2 = vBuffer[i2];

    float2 packedBary = RestirUnpackBarycentrics(sample.reconnectBarycentrics);
    float3 bary = float3(1.0f - packedBary.x - packedBary.y, packedBary.x, packedBary.y);

    float3 positionObj = v0.position * bary.x + v1.position * bary.y + v2.position * bary.z;
    float3 normalObj =
        RestirSafeNormalize(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z, float3(0.0f, 1.0f, 0.0f));

    position = mul(float4(positionObj, 1.0f), inst.worldMatrix).xyz;
    normal = RestirSafeNormalize(mul(normalObj, (float3x3) inst.worldInverse), float3(0.0f, 1.0f, 0.0f));
    return true;
}

bool RestirValidateTemporalSurface(RestirPrimaryHit currentHit, RestirPrimaryHit previousHit)
{
    if (!RestirIsValidPrimaryHit(currentHit) || !RestirIsValidPrimaryHit(previousHit))
    {
        return false;
    }

    if (currentHit.instanceId != previousHit.instanceId ||
        currentHit.materialId != previousHit.materialId ||
        currentHit.geometryId != previousHit.geometryId)
    {
        return false;
    }

    float3 currentNormal = RestirUnpackNormalOct16(currentHit.packedNormal);
    float3 previousNormal = RestirUnpackNormalOct16(previousHit.packedNormal);
    if (dot(currentNormal, previousNormal) < g_restirTemporalConstants.normalThreshold)
    {
        return false;
    }

    float positionThreshold =
        max(0.05f, abs(currentHit.positionDistance.w) * g_restirTemporalConstants.positionThresholdScale);
    return length(currentHit.positionDistance.xyz - previousHit.positionDistance.xyz) <= positionThreshold;
}

bool RestirTraceVisibility(float3 origin, float3 normal, float3 target)
{
    float3 delta = target - origin;
    float distance = length(delta);
    if (distance <= 0.03f)
    {
        return false;
    }

    RayQuery <
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_FORCE_OPAQUE |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
    > rq;

    RayDesc ray;
    ray.Origin = origin + normal * 0.01f;
    ray.Direction = delta / distance;
    ray.TMin = RAY_TMIN;
    ray.TMax = max(RAY_TMIN, distance - 0.02f);

    rq.TraceRayInline(g_scene, RAY_FLAG_NONE, 0xFF, ray);
    while (rq.Proceed())
    {
        if (rq.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            rq.CommitNonOpaqueTriangleHit();
        }
    }

    return rq.CommittedStatus() == COMMITTED_NOTHING;
}

bool RestirMakeCurrentTemporalCandidate(
    RestirReservoir sourceReservoir,
    RestirPrimaryHit currentHit,
    out RestirPathSample candidate,
    out float resamplingWeight,
    out float targetMass)
{
    candidate = sourceReservoir.sample;
    resamplingWeight = 0.0f;
    targetMass = 0.0f;

    if (!RestirIsValidReservoir(sourceReservoir) || !RestirIsValidPrimaryHit(currentHit))
    {
        return false;
    }

    float contributionWeight = RestirContributionWeightFromReservoir(sourceReservoir);
    float target = RestirTargetFromPathContribution(candidate.contributionTarget.rgb);
    if (target <= 0.0f || contributionWeight <= 0.0f)
    {
        return false;
    }

    candidate.sourceKind = RESTIR_SOURCE_TEMPORAL_REUSE;
    candidate.shiftKind = RESTIR_SHIFT_IDENTITY;
    candidate.weightTerms = float4(target, contributionWeight, 1.0f, 1.0f);

    float confidence = max(1.0f, float(sourceReservoir.sampleCount));
    targetMass = target * confidence;
    resamplingWeight = target * contributionWeight;
    return true;
}

bool RestirMakeReconnectedTemporalCandidate(
    RestirReservoir sourceReservoir,
    RestirPrimaryHit currentHit,
    RestirPrimaryHit previousHit,
    float cappedSourceM,
    out RestirPathSample candidate,
    out float resamplingWeight,
    out float targetMass)
{
    candidate = sourceReservoir.sample;
    resamplingWeight = 0.0f;
    targetMass = 0.0f;

    if (!RestirIsValidReservoir(sourceReservoir) || !RestirIsValidPrimaryHit(currentHit) ||
        !RestirIsValidPrimaryHit(previousHit))
    {
        return false;
    }

    float3 currentPos = currentHit.positionDistance.xyz;
    float3 currentNormal = RestirUnpackNormalOct16(currentHit.packedNormal);
    float3 sourceFirstPos = previousHit.positionDistance.xyz;
    float3 reconnectPos;
    float3 reconnectNormal;
    if (!RestirLoadReconnectSurface(sourceReservoir.sample, reconnectPos, reconnectNormal))
    {
        return false;
    }

    float3 sourceToReconnect = reconnectPos - sourceFirstPos;
    float3 currentToReconnect = reconnectPos - currentPos;
    float sourceDistance = length(sourceToReconnect);
    float currentDistance = length(currentToReconnect);
    if (sourceDistance <= 0.03f || currentDistance <= 0.03f)
    {
        return false;
    }

    float sourceGeometry = abs(dot(reconnectNormal, -sourceToReconnect / sourceDistance)) /
                           max(sourceDistance * sourceDistance, EPSILON);
    float currentGeometry = abs(dot(reconnectNormal, -currentToReconnect / currentDistance)) /
                            max(currentDistance * currentDistance, EPSILON);
    float shiftJacobian = currentGeometry / max(sourceGeometry, EPSILON);
    if (!RestirIsFinitePositive(shiftJacobian))
    {
        return false;
    }

    float jacobianLimit = 1.0f + g_restirTemporalConstants.jacobianRejectionThreshold;
    if (max(shiftJacobian, rcp(max(shiftJacobian, EPSILON))) > jacobianLimit)
    {
        return false;
    }

    if (!RestirTraceVisibility(currentPos, currentNormal, reconnectPos))
    {
        return false;
    }

    // Approx v0.5: raw-anchor reconnection only. This does not rebuild the shifted
    // BSDF/pdf/integrand terms and must be replaced by the v2 hit-ref layout before
    // treating temporal history accumulation as the final Talbot-style estimator.
    float contributionWeight = RestirContributionWeightFromReservoir(sourceReservoir);
    float target = RestirTargetFromPathContribution(candidate.contributionTarget.rgb);
    if (target <= 0.0f || contributionWeight <= 0.0f || cappedSourceM <= 0.0f)
    {
        return false;
    }

    candidate.sourceKind = RESTIR_SOURCE_TEMPORAL_REUSE;
    candidate.shiftKind = RESTIR_SHIFT_RECONNECTION;
    candidate.weightTerms = float4(target, contributionWeight, 1.0f, shiftJacobian);

    targetMass = target * cappedSourceM;
    resamplingWeight = target * contributionWeight * shiftJacobian;
    return true;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= g_restirTemporalConstants.screenWidth || pixelCoord.y >= g_restirTemporalConstants.screenHeight)
    {
        return;
    }

    uint pixelIndex = pixelCoord.y * g_restirTemporalConstants.screenWidth + pixelCoord.x;
    RestirReservoir currentReservoir = g_restirReservoirInitial[pixelIndex];
    RestirPrimaryHit currentHit = g_restirPrimaryHitCurrent[pixelIndex];

    RestirReservoir outputReservoir = RestirMakeEmptyReservoir();
    RestirPathSample currentCandidate;
    float currentWeight;
    float currentMass;
    bool hasCurrentCandidate = RestirMakeCurrentTemporalCandidate(
        currentReservoir,
        currentHit,
        currentCandidate,
        currentWeight,
        currentMass
    );
    bool hasCurrentSurface = RestirIsValidPrimaryHit(currentHit);

    RestirPathSample reconnectedCandidate;
    float reconnectedWeight = 0.0f;
    float reconnectedMass = 0.0f;
    float cappedHistoryM = 0.0f;
    bool hasReconnectedCandidate = false;

    if (0u != g_restirTemporalConstants.historyValid && hasCurrentSurface)
    {
        float2 motionVector = g_restirMotionVector.Load(int3(pixelCoord, 0));
        float2 previousPixelFloat = float2(pixelCoord) +
                                    motionVector * float2(
                                        g_restirTemporalConstants.screenWidth,
                                        g_restirTemporalConstants.screenHeight
                                    );
        int2 previousPixel = int2(floor(previousPixelFloat + 0.5f));

        if (previousPixel.x >= 0 && previousPixel.y >= 0 &&
            previousPixel.x < int(g_restirTemporalConstants.screenWidth) &&
            previousPixel.y < int(g_restirTemporalConstants.screenHeight))
        {
            uint previousIndex = uint(previousPixel.y) * g_restirTemporalConstants.screenWidth + uint(previousPixel.x);
            RestirPrimaryHit previousHit = g_restirPrimaryHitHistoryRead[previousIndex];
            RestirReservoir previousReservoir = g_restirReservoirHistoryRead[previousIndex];

            if (RestirValidateTemporalSurface(currentHit, previousHit) && RestirIsValidReservoir(previousReservoir))
            {
                float currentM = max(1.0f, float(currentReservoir.sampleCount));
                cappedHistoryM =
                    min(float(previousReservoir.sampleCount), g_restirTemporalConstants.temporalMCap * currentM);
                hasReconnectedCandidate = RestirMakeReconnectedTemporalCandidate(
                    previousReservoir,
                    currentHit,
                    previousHit,
                    cappedHistoryM,
                    reconnectedCandidate,
                    reconnectedWeight,
                    reconnectedMass
                );
            }
        }
    }

    uint rngSeed = pixelIndex * 9781u ^ g_restirTemporalConstants.frameSeed * 104729u ^ 0x9e3779b9u;
    if (hasCurrentCandidate || hasReconnectedCandidate)
    {
        float currentCandidateMass = hasCurrentCandidate ? currentMass : 0.0f;
        float massSum = max(currentCandidateMass + reconnectedMass, EPSILON);
        if (hasCurrentCandidate)
        {
            currentCandidate.weightTerms.z = hasReconnectedCandidate ? currentMass / massSum : 1.0f;
            RestirUpdateReservoir(
                outputReservoir,
                currentCandidate,
                currentWeight * currentCandidate.weightTerms.z,
                RestirRandom(rngSeed)
            );
        }

        if (hasReconnectedCandidate)
        {
            reconnectedCandidate.weightTerms.z = reconnectedMass / massSum;
            RestirUpdateReservoir(
                outputReservoir,
                reconnectedCandidate,
                reconnectedWeight * reconnectedCandidate.weightTerms.z,
                RestirRandom(rngSeed)
            );
        }

        outputReservoir.sampleCount = max(
            1u,
            (hasCurrentSurface ? currentReservoir.sampleCount : 0u) +
                (hasReconnectedCandidate ? (uint) max(1.0f, cappedHistoryM) : 0u)
        );
    }

    g_restirFinalReservoirWrite[pixelIndex] = outputReservoir;
    // Approx v0.5 output is intentionally not accumulated into history.
    // Store only the fresh current-frame reservoir until the v2 hit-ref shift is in place.
    g_restirReservoirHistoryWrite[pixelIndex] = currentReservoir;
    g_restirPrimaryHitHistoryWrite[pixelIndex] = currentHit;
}
