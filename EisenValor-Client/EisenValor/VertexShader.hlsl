// VertexShader.hlsl
// 상수 버퍼 추가
cbuffer ConstantBuffer : register(b0)
{
    matrix mvp;
}

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    
    //MVP 행렬 적용
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.color = input.color;
    
    return output;
}