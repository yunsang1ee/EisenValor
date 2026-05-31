#define HLSL
#include "RaytracingCommon.h"
#include "RestirReservoir.hlsli"

StructuredBuffer<RestirReservoir> g_restirFinalReservoir : register(t0, space0);
StructuredBuffer<RestirPrimaryHit> g_restirPrimaryHitCurrent : register(t1, space0);
RWTexture2D<float4> g_output : register(u0, space0);

cbuffer RestirFinalEvaluationConstants : register(b0, space0)
{
    uint g_screenWidth;
    uint g_screenHeight;
    uint g_restirDebugView;
    uint g_pad1;
};

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

    if (0u != g_restirDebugView)
    {
        RestirPrimaryHit selectedHit = RestirPrimaryHitFromSample(reservoir.sample, reservoir.flags);
        RestirPrimaryHit currentHit = g_restirPrimaryHitCurrent[pixelIndex];
        float3 debugColor = RestirDebugColor(reservoir, selectedHit, currentHit, g_restirDebugView);
        g_output[pixelCoord] = float4(max(0.0f.xxx, debugColor), 1.0f);
        return;
    }

    float3 color = 0.0f.xxx;
    if (0u != (reservoir.flags & RESTIR_RESERVOIR_VALID) && reservoir.sampleCount > 0u)
    {
        float selectedTarget = max(reservoir.sample.weightTerms.x, EPSILON);
        float normalization = reservoir.resamplingWeightSum / (selectedTarget * float(reservoir.sampleCount));
        color = reservoir.sample.contributionTarget.rgb * normalization;
    }

    g_output[pixelCoord] = float4(max(0.0f.xxx, color), 1.0f);
}
