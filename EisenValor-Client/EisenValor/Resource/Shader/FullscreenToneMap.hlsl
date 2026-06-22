#include "RaytracingPostProcess.hlsli"

Texture2D<float4> g_hdrInput : register(t0, space0);

cbuffer FullscreenResolveConstants : register(b0, space0)
{
    uint g_bypassToneMap;
    uint g_pad0;
    uint g_pad1;
    uint g_pad2;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);

    VSOutput output;
    output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    output.uv = uv;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    uint width;
    uint height;
    g_hdrInput.GetDimensions(width, height);

    uint2 pixel = min(uint2(input.uv * float2(width, height)), uint2(width - 1u, height - 1u));
    float3 color = g_hdrInput.Load(int3(pixel, 0)).rgb;

    if (0u != g_bypassToneMap)
    {
        return float4(saturate(color), 1.0f);
    }

    color = ToneMapACES(color);
    color = pow(color, (1.0f / 2.2f).xxx);

    return float4(saturate(color), 1.0f);
}
