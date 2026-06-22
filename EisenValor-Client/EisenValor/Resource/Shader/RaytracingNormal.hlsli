#ifndef RAYTRACING_NORMAL_HLSLI
#define RAYTRACING_NORMAL_HLSLI

float3 SafeNormalizeRay(float3 value, float3 fallback)
{
    float lenSq = dot(value, value);
    return (lenSq > 1e-10f) ? value * rsqrt(lenSq) : fallback;
}

void BuildFallbackTangentFrame(float3 normal, out float3 tangent, out float3 bitangent)
{
    float3 up = (abs(normal.y) < 0.999f) ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    tangent = SafeNormalizeRay(cross(up, normal), float3(1.0f, 0.0f, 0.0f));
    bitangent = cross(normal, tangent);
}

void BuildSafeTangentFrame(float3 normal, float3 tangentCandidate, float tangentSign, out float3 tangent, out float3 bitangent)
{
    normal = SafeNormalizeRay(normal, float3(0.0f, 1.0f, 0.0f));

    float3 fallbackTangent;
    float3 fallbackBitangent;
    BuildFallbackTangentFrame(normal, fallbackTangent, fallbackBitangent);

    tangent = tangentCandidate - normal * dot(tangentCandidate, normal);
    tangent = SafeNormalizeRay(tangent, fallbackTangent);

    float sign = (tangentSign < 0.0f) ? -1.0f : 1.0f;
    bitangent = SafeNormalizeRay(cross(normal, tangent) * sign, fallbackBitangent * sign);
}

float3 TangentNormalToWorld(float3 normalTS, float3 tangent, float3 bitangent, float3 normal)
{
    return SafeNormalizeRay(
        tangent * normalTS.x +
        bitangent * normalTS.y +
        normal * normalTS.z,
        normal
    );
}

#endif // RAYTRACING_NORMAL_HLSLI
