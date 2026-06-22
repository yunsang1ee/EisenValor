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
StructuredBuffer<MaterialGPUData> g_materials : register(t9, space0);
StructuredBuffer<TerrainSurfaceGPUData> g_terrainSurfaces : register(t10, space0);
RWStructuredBuffer<RestirReservoir> g_restirFinalReservoirWrite : register(u0, space0);
RWStructuredBuffer<RestirReservoir> g_restirReservoirHistoryWrite : register(u1, space0);
RWStructuredBuffer<RestirPrimaryHit> g_restirPrimaryHitHistoryWrite : register(u2, space0);

cbuffer RestirTemporalConstantsBuffer : register(b0, space0)
{
    RestirTemporalConstants g_restirTemporalConstants;
};

SamplerState g_sampler : register(s0, space0);

#include "RestirShading.hlsli"

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

bool RestirLoadReconnectSurface(RestirPathSample sample, out RestirSurface surface)
{
    surface = (RestirSurface)0;

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

    if (!RestirLoadSurface(
            instanceIndex,
            sample.reconnectGeometryIndex,
            sample.reconnectPrimitiveIndex,
            sample.reconnectBarycentrics,
            g_restirTemporalConstants.shadingNormalStrength,
            surface))
    {
        return false;
    }
    return true;
}

bool RestirLoadPrimarySurface(RestirPrimaryHit hit, out RestirSurface surface)
{
    surface = (RestirSurface)0;
    if (!RestirIsValidPrimaryHit(hit) ||
        hit.instanceId >= g_restirTemporalConstants.idToInstanceIndexCount)
    {
        return false;
    }

    uint instanceIndex = g_idToInstanceIndex[hit.instanceId];
    if (instanceIndex >= g_restirTemporalConstants.instanceCount)
    {
        return false;
    }

    InstanceData inst = g_instanceBuffer[instanceIndex];
    GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + hit.geometryIndex];
    MaterialGPUData material = g_materials[geo.materialIdx];
    if (geo.stableGeometryId != hit.geometryId || material.stableMaterialId != hit.materialId)
    {
        return false;
    }

    return RestirLoadSurface(
        instanceIndex,
        hit.geometryIndex,
        hit.primitiveIndex,
        hit.barycentrics,
        g_restirTemporalConstants.shadingNormalStrength,
        surface
    );
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

    // self-hit 방지: offset된 origin 기준으로 direction/tMax를 일관되게 계산하고 양끝 마진 확대
    float3 rayOrigin = origin + normal * 0.02f;
    float3 toTarget = target - rayOrigin;
    float traceDistance = length(toTarget);

    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = toTarget / max(traceDistance, EPSILON);
    ray.TMin = RAY_TMIN;
    ray.TMax = max(RAY_TMIN, traceDistance - 0.05f);

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

bool RestirTraceDirectionalVisibility(float3 origin, float3 normal, float3 direction)
{
    RayQuery <
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_FORCE_OPAQUE |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
    > rq;

    RayDesc ray;
    ray.Origin = origin + normal * 0.01f;
    ray.Direction = normalize(direction);
    ray.TMin = RAY_TMIN;
    ray.TMax = RAY_TMAX;

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

void RestirOrientSurfaceToView(inout RestirSurface surface, float3 viewDirection)
{
    if (dot(surface.normal, viewDirection) < 0.0f)
    {
        surface.normal = -surface.normal;
    }
    if (dot(surface.shadingNormal, viewDirection) < 0.0f)
    {
        surface.shadingNormal = -surface.shadingNormal;
    }
}

bool RestirIsFiniteContribution(float3 contribution)
{
    static const float maxFinite = 3.402823466e+38f;
    return all(contribution >= 0.0f.xxx) &&
           all(contribution < maxFinite.xxx) &&
           any(contribution > 0.0f.xxx);
}

struct RestirShiftResult
{
    float3 contribution;
    float jacobian;
    float destinationPdfBefore;
    float destinationPdfAfter;
    float destinationGeometry;
    float destinationLightPdf;
};

RestirShiftResult RestirMakeEmptyShiftResult()
{
    RestirShiftResult result;
    result.contribution = 0.0f.xxx;
    result.jacobian = 0.0f;
    result.destinationPdfBefore = 0.0f;
    result.destinationPdfAfter = 0.0f;
    result.destinationGeometry = 0.0f;
    result.destinationLightPdf = 0.0f;
    return result;
}

bool RestirEvaluateReconnectionShift(
    RestirPathSample sourceSample,
    RestirSurface currentSurface,
    float3 destinationCameraPosition,
    out RestirShiftResult result)
{
    result = RestirMakeEmptyShiftResult();

    uint pathFlags = RestirGetPathFlags(sourceSample);
    uint lobeBefore = RestirGetLobeBeforeRc(sourceSample);
    if (lobeBefore == 0u || RestirIsDeltaBeforeRc(sourceSample) || RestirIsDeltaAfterRc(sourceSample))
    {
        return false;
    }

    float3 currentPos = currentSurface.position;
    float3 primaryView = SafeNormalizeRay(
        destinationCameraPosition - currentPos,
        currentSurface.shadingNormal
    );
    RestirOrientSurfaceToView(currentSurface, primaryView);

    float3 rcWi = RestirGetRcVertexWi(sourceSample);
    float3 rcIrradiance = RestirGetRcVertexIrradiance(sourceSample);
    float sourcePdfBefore = RestirGetSourcePdfBeforeRc(sourceSample);
    float sourcePdfAfter = RestirGetSourcePdfAfterRc(sourceSample);
    float sourceGeometry = RestirGetSourceGeometry(sourceSample);
    float sourceLightPdf = RestirGetLightPdf(sourceSample);
    bool hasReconnectHit = RestirHasReconnectHitRef(sourceSample);

    if (0u != (pathFlags & RESTIR_PATH_FLAG_NEE_SUN) && !hasReconnectHit)
    {
        float3 shiftedBsdf = RestirEvalBSDF(currentSurface, primaryView, rcWi, lobeBefore);
        float shiftedPdf = RestirEvalPdfBSDF(currentSurface, primaryView, rcWi, lobeBefore);
        if (sourceLightPdf <= 0.0f ||
            !RestirTraceDirectionalVisibility(currentPos, currentSurface.geometricNormal, rcWi))
        {
            return false;
        }

        result.contribution = shiftedBsdf * rcIrradiance / max(sourceLightPdf, EPSILON);
        result.jacobian = 1.0f;
        result.destinationPdfBefore = shiftedPdf;
        result.destinationGeometry = 1.0f;
        result.destinationLightPdf = sourceLightPdf;
        return RestirIsFiniteContribution(result.contribution);
    }

    if (0u != (pathFlags & RESTIR_PATH_FLAG_SKY_ESCAPE) && !hasReconnectHit)
    {
        float destinationPdf = RestirEvalPdfBSDF(currentSurface, primaryView, rcWi, lobeBefore);
        if (sourcePdfBefore <= 0.0f || destinationPdf <= 0.0f ||
            !RestirTraceDirectionalVisibility(currentPos, currentSurface.geometricNormal, rcWi))
        {
            return false;
        }

        float3 shiftedBsdf = RestirEvalBSDF(currentSurface, primaryView, rcWi, lobeBefore);
        result.contribution = shiftedBsdf * rcIrradiance / destinationPdf;
        result.jacobian = destinationPdf / sourcePdfBefore;
        result.destinationPdfBefore = destinationPdf;
        return RestirIsFiniteContribution(result.contribution) && RestirIsFinitePositive(result.jacobian);
    }

    if (!hasReconnectHit || !RestirIsFinitePositive(sourceGeometry))
    {
        return false;
    }

    RestirSurface reconnectSurface;
    if (!RestirLoadReconnectSurface(sourceSample, reconnectSurface))
    {
        return false;
    }

    float3 currentToReconnect = reconnectSurface.position - currentPos;
    float currentDistance = length(currentToReconnect);
    if (currentDistance <= 0.03f)
    {
        return false;
    }

    float3 connectionDirection = currentToReconnect / currentDistance;
    float reconnectCos = dot(reconnectSurface.faceNormal, -connectionDirection);
    if ((RestirIsRcFinal(sourceSample) || RestirIsRcNeeTerminal(sourceSample)) &&
        0u == (reconnectSurface.material.materialFlags & MATERIAL_FLAG_DOUBLE_SIDED))
    {
        reconnectCos = max(reconnectCos, 0.0f);
    }
    else
    {
        reconnectCos = abs(reconnectCos);
    }
    float currentGeometry = reconnectCos / max(currentDistance * currentDistance, EPSILON);
    if (!RestirIsFinitePositive(currentGeometry) ||
        !RestirTraceVisibility(currentPos, currentSurface.geometricNormal, reconnectSurface.position))
    {
        return false;
    }

    float3 shiftedBsdfBefore = RestirEvalBSDF(
        currentSurface,
        primaryView,
        connectionDirection,
        lobeBefore
    );
    float destinationPdfBefore = RestirEvalPdfBSDF(
        currentSurface,
        primaryView,
        connectionDirection,
        lobeBefore
    );
    if (destinationPdfBefore <= 0.0f)
    {
        return false;
    }

    float geometryRatio = currentGeometry / sourceGeometry;
    result.destinationPdfBefore = destinationPdfBefore;
    result.destinationGeometry = currentGeometry;

    if (RestirIsRcNeeTerminal(sourceSample))
    {
        if (sourceLightPdf <= 0.0f)
        {
            return false;
        }

        float destinationLightPdf = sourceLightPdf * sourceGeometry / currentGeometry;
        float misWeight = destinationLightPdf /
                          max(destinationLightPdf + destinationPdfBefore, EPSILON);
        result.contribution = shiftedBsdfBefore * rcIrradiance * misWeight /
                              max(destinationLightPdf, EPSILON);
        result.jacobian = geometryRatio;
        result.destinationLightPdf = destinationLightPdf;
    }
    else if (RestirIsRcFinal(sourceSample))
    {
        if (sourcePdfBefore <= 0.0f)
        {
            return false;
        }

        float misWeight = 1.0f;
        float destinationLightPdf = sourceLightPdf;
        if (sourceLightPdf > 0.0f)
        {
            destinationLightPdf = sourceLightPdf * sourceGeometry / currentGeometry;
            misWeight = destinationPdfBefore /
                        max(destinationPdfBefore + destinationLightPdf, EPSILON);
        }

        result.contribution = shiftedBsdfBefore * rcIrradiance * misWeight /
                              destinationPdfBefore;
        result.jacobian = geometryRatio * destinationPdfBefore / sourcePdfBefore;
        result.destinationLightPdf = destinationLightPdf;
    }
    else
    {
        uint lobeAfter = RestirGetLobeAfterRc(sourceSample);
        if (lobeAfter == 0u || sourcePdfBefore <= 0.0f || sourcePdfAfter <= 0.0f)
        {
            return false;
        }

        float3 reconnectView = -connectionDirection;
        RestirOrientSurfaceToView(reconnectSurface, reconnectView);
        float3 shiftedBsdfAfter = RestirEvalBSDF(
            reconnectSurface,
            reconnectView,
            rcWi,
            lobeAfter
        );
        float destinationPdfAfter = RestirEvalPdfBSDF(
            reconnectSurface,
            reconnectView,
            rcWi,
            lobeAfter
        );
        if (destinationPdfAfter <= 0.0f)
        {
            return false;
        }

        result.contribution =
            (shiftedBsdfBefore / destinationPdfBefore) *
            (shiftedBsdfAfter / destinationPdfAfter) *
            rcIrradiance;
        if (RestirRcSuffixNeedsEmissiveMis(sourceSample) && sourceLightPdf > 0.0f)
        {
            float misWeight = destinationPdfAfter /
                              max(destinationPdfAfter + sourceLightPdf, EPSILON);
            result.contribution *= misWeight;
        }
        result.jacobian =
            geometryRatio *
            (destinationPdfBefore / sourcePdfBefore) *
            (destinationPdfAfter / sourcePdfAfter);
        result.destinationPdfAfter = destinationPdfAfter;
        result.destinationLightPdf = sourceLightPdf;
    }

    return RestirIsFiniteContribution(result.contribution) && RestirIsFinitePositive(result.jacobian);
}

bool RestirAcceptShiftJacobian(float jacobian)
{
    if (!RestirIsFinitePositive(jacobian))
    {
        return false;
    }

    float jacobianLimit = 1.0f + g_restirTemporalConstants.jacobianRejectionThreshold;
    return max(jacobian, rcp(max(jacobian, EPSILON))) <= jacobianLimit;
}

bool RestirMakeCurrentTemporalCandidate(
    RestirReservoir sourceReservoir,
    RestirPrimaryHit currentHit,
    out RestirPathSample candidate,
    out float resamplingWeight)
{
    candidate = sourceReservoir.sample;
    resamplingWeight = 0.0f;

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

    RestirSetSampleSourceKind(candidate, RESTIR_SOURCE_TEMPORAL_REUSE);
    RestirSetSampleShiftKind(candidate, RESTIR_SHIFT_IDENTITY);
    candidate.weightTerms = float4(target, contributionWeight, 1.0f, 1.0f);

    resamplingWeight = target * contributionWeight;
    return true;
}

bool RestirMakeReconnectedTemporalCandidate(
    RestirReservoir sourceReservoir,
    RestirPrimaryHit currentHit,
    out RestirPathSample candidate,
    out float resamplingWeight)
{
    candidate = sourceReservoir.sample;
    resamplingWeight = 0.0f;

    if (!RestirIsValidReservoir(sourceReservoir) || !RestirIsValidPrimaryHit(currentHit) ||
        !RestirHasShiftData(sourceReservoir.sample))
    {
        return false;
    }

    RestirSurface currentSurface;
    if (!RestirLoadPrimarySurface(currentHit, currentSurface))
    {
        return false;
    }

    RestirShiftResult shiftResult;
    if (!RestirEvaluateReconnectionShift(
            sourceReservoir.sample,
            currentSurface,
            g_restirTemporalConstants.cameraPosition,
            shiftResult))
    {
        return false;
    }

    if (!RestirAcceptShiftJacobian(shiftResult.jacobian))
    {
        return false;
    }

    float contributionWeight = RestirContributionWeightFromReservoir(sourceReservoir);
    float target = RestirTargetFromPathContribution(shiftResult.contribution);
    if (target <= 0.0f || contributionWeight <= 0.0f)
    {
        return false;
    }

    candidate.contributionTarget = float4(shiftResult.contribution, shiftResult.destinationGeometry);
    candidate.throughputPdf.x = shiftResult.destinationLightPdf;
    candidate.throughputPdf.z = shiftResult.destinationPdfBefore;
    candidate.throughputPdf.w = shiftResult.destinationPdfAfter;
    RestirSetSampleSourceKind(candidate, RESTIR_SOURCE_TEMPORAL_REUSE);
    RestirSetSampleShiftKind(candidate, RESTIR_SHIFT_RECONNECTION);
    candidate.weightTerms = float4(target, contributionWeight, 1.0f, shiftResult.jacobian);

    resamplingWeight = target * contributionWeight * shiftResult.jacobian;
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

    if (RestirIsSkyEscapeReservoir(currentReservoir) && !RestirIsValidPrimaryHit(currentHit))
    {
        g_restirFinalReservoirWrite[pixelIndex] = currentReservoir;
        g_restirReservoirHistoryWrite[pixelIndex] = currentReservoir;
        g_restirPrimaryHitHistoryWrite[pixelIndex] = currentHit;
        return;
    }

    RestirReservoir outputReservoir = RestirMakeEmptyReservoir();
    RestirPathSample currentCandidate;
    float currentWeight;
    bool hasCurrentCandidate = RestirMakeCurrentTemporalCandidate(
        currentReservoir,
        currentHit,
        currentCandidate,
        currentWeight
    );
    bool hasCurrentSurface = RestirIsValidPrimaryHit(currentHit);

    RestirPathSample reconnectedCandidate;
    float reconnectedWeight = 0.0f;
    float cappedHistoryM = 0.0f;
    bool hasReconnectedCandidate = false;
    bool hasPreviousDomain = false;
    RestirPrimaryHit previousHit = RestirMakeInvalidPrimaryHit();
    RestirReservoir previousReservoir = RestirMakeEmptyReservoir();
    uint rngSeed = pixelIndex * 9781u ^ g_restirTemporalConstants.frameSeed * 104729u ^ 0x9e3779b9u;

    if (0u != g_restirTemporalConstants.historyValid && hasCurrentSurface)
    {
        float2 motionVector = g_restirMotionVector.Load(int3(pixelCoord, 0));
        float2 previousPixelFloat = float2(pixelCoord) +
                                    motionVector * float2(
                                        g_restirTemporalConstants.screenWidth,
                                        g_restirTemporalConstants.screenHeight
                                    );
        float2 reprojectionRandom = float2(RestirRandom(rngSeed), RestirRandom(rngSeed));
        int2 previousPixel = int2(floor(previousPixelFloat + reprojectionRandom));

        if (previousPixel.x >= 0 && previousPixel.y >= 0 &&
            previousPixel.x < int(g_restirTemporalConstants.screenWidth) &&
            previousPixel.y < int(g_restirTemporalConstants.screenHeight))
        {
            uint previousIndex = uint(previousPixel.y) * g_restirTemporalConstants.screenWidth + uint(previousPixel.x);
            previousHit = g_restirPrimaryHitHistoryRead[previousIndex];
            previousReservoir = g_restirReservoirHistoryRead[previousIndex];

            if (RestirValidateTemporalSurface(currentHit, previousHit) && RestirIsValidReservoir(previousReservoir))
            {
                float currentM = max(1.0f, float(currentReservoir.sampleCount));
                cappedHistoryM =
                    min(float(previousReservoir.sampleCount), g_restirTemporalConstants.temporalMCap * currentM);
                hasPreviousDomain = cappedHistoryM > 0.0f;
                hasReconnectedCandidate = RestirMakeReconnectedTemporalCandidate(
                    previousReservoir,
                    currentHit,
                    reconnectedCandidate,
                    reconnectedWeight
                );
            }
        }
    }

    float backwardTarget = 0.0f;
    float backwardJacobian = 0.0f;
    bool hasBackwardShift = false;
    if (hasCurrentCandidate && hasPreviousDomain && RestirHasShiftData(currentReservoir.sample))
    {
        RestirSurface previousSurface;
        RestirShiftResult backwardResult;
        if (RestirLoadPrimarySurface(previousHit, previousSurface) &&
            RestirEvaluateReconnectionShift(
                currentReservoir.sample,
                previousSurface,
                g_restirTemporalConstants.previousCameraPosition,
                backwardResult) &&
            RestirAcceptShiftJacobian(backwardResult.jacobian))
        {
            backwardTarget = RestirTargetFromPathContribution(backwardResult.contribution);
            backwardJacobian = backwardResult.jacobian;
            hasBackwardShift = backwardTarget > 0.0f;
        }
    }

    if (hasCurrentCandidate || hasReconnectedCandidate)
    {
        float currentM = max(1.0f, float(currentReservoir.sampleCount));
        if (hasCurrentCandidate)
        {
            float currentSelfDensity = currentCandidate.weightTerms.x * currentM;
            float previousDomainDensity = hasBackwardShift
                                              ? backwardTarget * backwardJacobian * cappedHistoryM
                                              : 0.0f;
            float currentMisWeight = currentSelfDensity /
                                     max(currentSelfDensity + previousDomainDensity, EPSILON);
            currentCandidate.weightTerms.z = currentMisWeight;
            RestirUpdateReservoir(
                outputReservoir,
                currentCandidate,
                currentWeight * currentMisWeight,
                RestirRandom(rngSeed)
            );
        }

        if (hasReconnectedCandidate)
        {
            float forwardTarget = reconnectedCandidate.weightTerms.x;
            float forwardJacobian = reconnectedCandidate.weightTerms.w;
            float previousTarget = RestirTargetFromPathContribution(
                previousReservoir.sample.contributionTarget.rgb
            );
            float currentDomainDensity = forwardTarget * currentM;
            float previousSelfDensity = previousTarget /
                                        max(forwardJacobian, EPSILON) * cappedHistoryM;
            float previousMisWeight = previousSelfDensity /
                                      max(currentDomainDensity + previousSelfDensity, EPSILON);
            reconnectedCandidate.weightTerms.z = previousMisWeight;
            RestirUpdateReservoir(
                outputReservoir,
                reconnectedCandidate,
                reconnectedWeight * previousMisWeight,
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
    // The stored sample is already expressed in the current pixel domain by the Talbot combine.
    g_restirReservoirHistoryWrite[pixelIndex] = outputReservoir;
    g_restirPrimaryHitHistoryWrite[pixelIndex] = currentHit;
}
