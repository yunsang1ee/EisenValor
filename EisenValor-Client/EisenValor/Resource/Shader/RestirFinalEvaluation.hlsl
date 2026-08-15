#define HLSL
#include "RaytracingCommon.h"
#include "RestirReservoir.hlsli"

StructuredBuffer<RestirReservoir> g_restirFinalReservoir : register(t0, space0);
#if RESTIR_ENABLE_DEBUG_VIEWS
Texture2D<float2> g_restirMotionVectors : register(t2, space0);
Texture2D<float> g_restirLinearDepth : register(t3, space0);
Texture2D<float4> g_restirDiffuseAlbedo : register(t4, space0);
Texture2D<float4> g_restirSpecularAlbedo : register(t5, space0);
Texture2D<float4> g_restirNormalRoughness : register(t6, space0);
#endif
RWTexture2D<float4> g_output : register(u0, space0);

cbuffer RestirFinalEvaluationConstants : register(b0, space0)
{
    uint g_screenWidth;
    uint g_screenHeight;
#if RESTIR_ENABLE_DEBUG_VIEWS
    uint g_restirDebugView;
    float g_cameraFarZ;
#endif
};

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
        g_output[pixelCoord] = float4(max(0.0f.xxx, debugColor), 1.0f);
        return;
    }
#endif

    float3 color = 0.0f.xxx;
    if (0u != (reservoir.flags & RESTIR_RESERVOIR_VALID) && reservoir.sampleCount > 0u)
    {
        color = reservoir.sample.contributionTarget.rgb * RestirContributionWeightFromReservoir(reservoir);
    }

    g_output[pixelCoord] = float4(max(0.0f.xxx, color), 1.0f);
}
