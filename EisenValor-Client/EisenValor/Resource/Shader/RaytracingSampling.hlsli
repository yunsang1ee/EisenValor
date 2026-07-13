#ifndef RAYTRACING_SAMPLING_HLSLI
#define RAYTRACING_SAMPLING_HLSLI

#include "RaytracingCommon.h"

float RandomValue(inout uint seed)
{
    seed = seed * 747796405u + 2891336453u;
    uint temp = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    temp = (temp >> 22u) ^ temp;
    return float(temp) / 4294967296.0f;
}

float2 RandomPointInCircle(inout uint seed)
{
    float angle = RandomValue(seed) * 2.0f * PI;
    float2 circle = float2(cos(angle), sin(angle));
    float radius = sqrt(RandomValue(seed));
    return circle * radius;
}

#endif // RAYTRACING_SAMPLING_HLSLI
