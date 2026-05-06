#ifndef RAYTRACING_MATERIAL_EMISSION_HLSLI
#define RAYTRACING_MATERIAL_EMISSION_HLSLI

static const uint EMISSION_VIEW_ART_DIRECTED = 0u;
static const uint EMISSION_VIEW_PHYSICAL = 1u;

float3 EvaluateMaterialEmission(MaterialGPUData mat, float2 uv, SamplerState materialSampler)
{
    float3 emission = mat.emissive.rgb * mat.emissive.a;
    if (0 != (mat.materialFlags & MATERIAL_FLAG_EMISSIVE_MAP))
    {
        Texture2D emissiveTexture = ResourceDescriptorHeap[mat.emissiveTextureIdx];
        float3 emissiveTexel = emissiveTexture.SampleLevel(materialSampler, uv, 0).rgb;
        float hasEmissionFactor = max(max(mat.emissive.x, mat.emissive.y), max(mat.emissive.z, mat.emissive.a)) > 0.0f ? 1.0f : 0.0f;
        float3 emissionFactor = lerp(1.0f.xxx, mat.emissive.rgb * mat.emissive.a, hasEmissionFactor);
        emission = emissiveTexel * emissionFactor;
    }
    return emission;
}

float3 EvaluateVisibleMaterialEmission(MaterialGPUData mat, float2 uv, SamplerState materialSampler)
{
    float3 emission = mat.visibleEmissive.rgb * mat.visibleEmissive.a;
    if (0 != (mat.materialFlags & MATERIAL_FLAG_EMISSIVE_MAP))
    {
        Texture2D emissiveTexture = ResourceDescriptorHeap[mat.emissiveTextureIdx];
        float3 emissiveTexel = emissiveTexture.SampleLevel(materialSampler, uv, 0).rgb;
        float hasEmissionFactor =
            max(max(mat.visibleEmissive.x, mat.visibleEmissive.y), max(mat.visibleEmissive.z, mat.visibleEmissive.a)) > 0.0f ? 1.0f : 0.0f;
        float3 emissionFactor = lerp(1.0f.xxx, mat.visibleEmissive.rgb * mat.visibleEmissive.a, hasEmissionFactor);
        emission = emissiveTexel * emissionFactor;
    }
    return emission;
}

float3 EvaluateCameraVisibleMaterialEmission(MaterialGPUData mat, float2 uv, uint emissionViewMode, SamplerState materialSampler)
{
    return (EMISSION_VIEW_PHYSICAL == emissionViewMode)
        ? EvaluateMaterialEmission(mat, uv, materialSampler)
        : EvaluateVisibleMaterialEmission(mat, uv, materialSampler);
}

#endif // RAYTRACING_MATERIAL_EMISSION_HLSLI
