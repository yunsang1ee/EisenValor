struct VSInput
{
    float2 position : POSITION;
};

// 인스턴스 입력 (슬롯 1) - C++ UIInstanceData와 레이아웃 일치
struct VSInstanceInput
{
    float4 transform0 : TRANSFORM0; 
    float4 transform1 : TRANSFORM1; 
    float4 transform2 : TRANSFORM2; 
    float4 transform3 : TRANSFORM3; 
    float4 color : COLOR;
    float2 uvMin : UV_MIN;   
    float2 uvMax : UV_MAX;   
    uint textureIndex : TEXTURE_INDEX;
    uint3 padding : PADDING;
    uint4 padding1 : PADDING1;  //128byre
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0; 
    float4 color : COLOR;
    uint textureIndex : TEXTURE_INDEX;  // 픽셀 셰이더로 전달
};


VSOutput main(VSInput input, VSInstanceInput instance)
{
    VSOutput output;

    float4x4 transform = float4x4(
        instance.transform0,
        instance.transform1,
        instance.transform2,
        instance.transform3
    );

    output.position = mul(transform, float4(input.position, 0.0f, 1.0f));

    // uvMin, uvMax를 사용하여 실제 UV 계산
    output.uv = instance.uvMin + input.position * (instance.uvMax - instance.uvMin);

    output.color = instance.color;
    output.textureIndex = instance.textureIndex;

    return output;
}