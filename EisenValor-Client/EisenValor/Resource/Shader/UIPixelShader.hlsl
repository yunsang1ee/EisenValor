struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    uint textureIndex : TEXTURE_INDEX;
};

// 텍스처 배열 (Bindless Rendering)
Texture2D g_textures[] : register(t0, space0);
SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    if (input.textureIndex == 0)
    {
        // 텍스처 없음 - 단색 렌더링
        return input.color;
    }
    else
    {
        // 실제 텍스처 업로드
        float4 texColor = g_textures[NonUniformResourceIndex(input.textureIndex)].Sample(g_sampler, input.uv);
        return texColor;
    }
}