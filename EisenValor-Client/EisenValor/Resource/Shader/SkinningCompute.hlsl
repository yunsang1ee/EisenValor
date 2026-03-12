#ifndef SKINNING_COMPUTE_HLSL
#define SKINNING_COMPUTE_HLSL

struct SkinnedVertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
    uint blendIndices;
    float4 blendWeights;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

StructuredBuffer<SkinnedVertex> InVertices : register(t0);
StructuredBuffer<float4x4> BoneMatrices : register(t1);
RWStructuredBuffer<Vertex> OutVertices : register(u0);

cbuffer SkinningConstants : register(b0)
{
    uint VertexCount;
    uint BoneBaseIndex;
    uint Pad0;
    uint Pad1;
};

[numthreads(256, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= VertexCount)
        return;

    SkinnedVertex v = InVertices[dtid.x];
    float3 pos = v.position;
    float3 normal = v.normal;
    float3 tangent = v.tangent.xyz;

    float4x4 m = float4x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    
    if (dot(v.blendWeights, 1.0f) > 0.001f)
    {
        uint b0 = (v.blendIndices >> 0) & 0xFF;
        uint b1 = (v.blendIndices >> 8) & 0xFF;
        uint b2 = (v.blendIndices >> 16) & 0xFF;
        uint b3 = (v.blendIndices >> 24) & 0xFF;

        m += v.blendWeights.x * BoneMatrices[BoneBaseIndex + b0];
        m += v.blendWeights.y * BoneMatrices[BoneBaseIndex + b1];
        m += v.blendWeights.z * BoneMatrices[BoneBaseIndex + b2];
        m += v.blendWeights.w * BoneMatrices[BoneBaseIndex + b3];
    }
    else
    {
        m = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    }

    Vertex outV;
    outV.position = mul(float4(pos, 1.0f), m).xyz;
    float3x3 m3x3 = (float3x3) m;
    outV.normal = normalize(mul(normal, m3x3));
    outV.tangent = float4(normalize(mul(tangent, m3x3)), v.tangent.w);
    outV.uv = v.uv;

    OutVertices[dtid.x] = outV;
}

#endif // SKINNING_COMPUTE_HLSL
