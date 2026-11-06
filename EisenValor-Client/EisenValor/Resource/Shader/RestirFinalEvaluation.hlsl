#define HLSL
#include "RaytracingCommon.h"
#include "RestirReservoir.hlsli"

StructuredBuffer<RestirReservoir> g_restirFinalReservoir : register(t0, space0);
StructuredBuffer<RestirReservoir> g_restirInitialReservoir : register(t1, space0);
#if RESTIR_ENABLE_DEBUG_VIEWS
Texture2D<float2> g_restirMotionVectors : register(t2, space0);
Texture2D<float> g_restirLinearDepth : register(t3, space0);
Texture2D<float4> g_restirDiffuseAlbedo : register(t4, space0);
Texture2D<float4> g_restirSpecularAlbedo : register(t5, space0);
Texture2D<float4> g_restirNormalRoughness : register(t6, space0);
Texture2D<float> g_restirSpecularHitDistance : register(t7, space0);
Texture2D<float4> g_restirSecondaryNee : register(t8, space0);
#endif
RWTexture2D<float4> g_output : register(u0, space0);

cbuffer RestirFinalEvaluationConstants : register(b0, space0)
{
    uint g_screenWidth;
    uint g_screenHeight;
    uint g_finalShadingFrameSeed;
    float g_rrDecorrelationFactor;
#if RESTIR_ENABLE_DEBUG_VIEWS
    uint g_restirDebugView;
    float g_cameraFarZ;
#endif
};

float RestirFinalShadingRandom(uint pixelIndex)
{
    // A separate stream from candidate construction and temporal reservoir selection.
    uint seed = pixelIndex * 747796405u ^ (g_finalShadingFrameSeed + 1u) * 2891336453u ^ 0x68bc21ebu;
    seed = seed * 747796405u + 2891336453u;
    uint value = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    value = (value >> 22u) ^ value;
    return float(value >> 8u) * (1.0f / 16777216.0f);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= g_screenWidth || pixelCoord.y >= g_screenHeight)
    {
        return;
    }

    uint pixelIndex = pixelCoord.y * g_screenWidth + pixelCoord.x;
    RestirReservoir reservoir = g_restirFinalReservoir[pixelIndex];

#if RESTIR_ENABLE_DEBUG_VIEWS
    if (0u != g_restirDebugView)
    {
        float3 debugColor = 0.0f.xxx;
        if (g_restirDebugView <= 6u)
        {
            debugColor = RestirDebugColor(reservoir, g_restirDebugView);
        }
        else if (7u == g_restirDebugView)
        {
            float2 motionVector = g_restirMotionVectors.Load(int3(pixelCoord, 0));
            debugColor = float3(saturate(float2(0.5f, 0.5f) + motionVector * 64.0f), 0.5f);
        }
        else if (8u == g_restirDebugView)
        {
            float linearDepth = max(0.0f, g_restirLinearDepth.Load(int3(pixelCoord, 0)));
            float normalizedDepth = log2(1.0f + linearDepth) / log2(1.0f + max(g_cameraFarZ, 1.0f));
            debugColor = saturate(normalizedDepth).xxx;
        }
        else if (9u == g_restirDebugView)
        {
            float4 normalRoughness = g_restirNormalRoughness.Load(int3(pixelCoord, 0));
            debugColor = float3(normalRoughness.xy * 0.5f + 0.5f, normalRoughness.w);
        }
        else if (10u == g_restirDebugView)
        {
            debugColor = saturate(g_restirDiffuseAlbedo.Load(int3(pixelCoord, 0)).rgb);
        }
        else if (11u == g_restirDebugView)
        {
            debugColor = saturate(g_restirSpecularAlbedo.Load(int3(pixelCoord, 0)).rgb);
        }
        else if (12u == g_restirDebugView)
        {
            float distance = g_restirSpecularHitDistance.Load(int3(pixelCoord, 0));
            debugColor = saturate(log2(1.0f + max(distance, 0.0f)) / log2(1.0f + RAY_TMAX)).xxx;
        }
        else if (13u == g_restirDebugView)
        {
            // Fresh camera-weighted NEE only, independent of the selected
            // reservoir. Keep HDR values; the regular tone mapper displays them.
            debugColor = g_restirSecondaryNee.Load(int3(pixelCoord, 0)).rgb;
            if (!all(isfinite(debugColor)))
            {
                debugColor = float3(1.0f, 0.0f, 1.0f);
            }
        }
        else if (14u == g_restirDebugView)
        {
            float3 secondaryNee = g_restirSecondaryNee.Load(int3(pixelCoord, 0)).rgb;
            RestirReservoir fresh = g_restirInitialReservoir[pixelIndex];
            float3 freshColor = 0.0f.xxx;
            if (RestirIsValidReservoir(fresh) && fresh.sampleCount > 0u)
            {
                freshColor = fresh.sample.contributionTarget.rgb * RestirContributionWeightFromReservoir(fresh);
            }
            float neeLuminance = RestirLuminance(secondaryNee);
            float totalLuminance = RestirLuminance(freshColor);
            // Fresh selection uses luminance as its target, so the selected
            // RGB's finalized luminance equals the sum of trial luminances / M
            // (away from the EPSILON guard). No temporal or RR color enters here.
            bool inconsistent = !all(isfinite(secondaryNee)) || !all(isfinite(freshColor)) ||
                !isfinite(neeLuminance) || !isfinite(totalLuminance) ||
                neeLuminance > totalLuminance * 1.01f + 1e-6f;
            debugColor = inconsistent ? float3(1.0f, 0.0f, 1.0f) :
                (totalLuminance > 0.0f ? saturate(neeLuminance / totalLuminance).xxx : 0.0f.xxx);
        }
        g_output[pixelCoord] = float4(max(0.0f.xxx, debugColor), 1.0f);
        return;
    }
#endif

    // Constant-probability final-shading decorrelation for RR. This only changes
    // the displayed estimator; the temporal reservoir and history remain intact.
    // Retain zero fresh samples too: replacing only valid ones would bias brightness.
    float decorrelationFactor = saturate(g_rrDecorrelationFactor);
    if (decorrelationFactor > 0.0f && RestirFinalShadingRandom(pixelIndex) < decorrelationFactor)
    {
        reservoir = g_restirInitialReservoir[pixelIndex];
    }

    float3 color = 0.0f.xxx;
    if (0u != (reservoir.flags & RESTIR_RESERVOIR_VALID) && reservoir.sampleCount > 0u)
    {
        color = reservoir.sample.contributionTarget.rgb * RestirContributionWeightFromReservoir(reservoir);
    }

    g_output[pixelCoord] = float4(max(0.0f.xxx, color), 1.0f);
}
