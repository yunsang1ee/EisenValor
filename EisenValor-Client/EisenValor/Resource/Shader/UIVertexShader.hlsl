struct VSInput
{
    float2 position : POSITION;
};

// 인스턴스 입력 (슬롯 1)
struct VSInstanceInput
{
    float4 transform0 : TRANSFORM0; 
    float4 transform1 : TRANSFORM1; 
    float4 transform2 : TRANSFORM2; 
    float4 transform3 : TRANSFORM3; 
    float4 color : COLOR;
    uint textureIndex : TEXTURE_INDEX;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0; // UV 좌표
    float4 color : COLOR;
    uint textureIndex : TEXTURE_INDEX; // 픽셀 셰이더로 전달
};


VSOutput main(VSInput input, VSInstanceInput instance)
{
    VSOutput output;
    
    // 인스턴스 데이터에서 행렬 구성
    float4x4 transform = float4x4(
        instance.transform0,
        instance.transform1,
        instance.transform2,
        instance.transform3
    );
    
    // transform 적용
    output.position = mul(transform, float4(input.position, 0.0f, 1.0f));
    output.uv = input.position; // 0~1 UV 좌표
    output.color = instance.color;
    output.textureIndex = instance.textureIndex;
    
    return output;
}