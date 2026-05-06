#ifndef RAYTRACING_POST_PROCESS_HLSLI
#define RAYTRACING_POST_PROCESS_HLSLI

float3 ToneMapACES(float3 color)
{
    color = max(0.0f.xxx, color);
    return saturate((color * (2.51f * color + 0.03f.xxx)) / (color * (2.43f * color + 0.59f.xxx) + 0.14f.xxx));
}

#endif // RAYTRACING_POST_PROCESS_HLSLI
