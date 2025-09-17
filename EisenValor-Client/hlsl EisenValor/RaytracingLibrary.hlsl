// =======================================================================================
// EisenValor Raytracing Library
// Ground reflection + Player emissive raytracing
// =======================================================================================

#define HLSL
#include "RaytracingCommon.hlsli"

// Global Root Signature
// Slot 0: Acceleration Structure (TLAS)
// Slot 1: Output UAV 
// Slot 2: Camera Constants

// Camera constants structure
cbuffer CameraConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjInverse;
    float3 cameraPosition;
    float _padding0;
    float3 cameraDirection;
    float _padding1;
};

// Global resources
RaytracingAccelerationStructure g_scene : register(t0);
RWTexture2D<float4> g_output : register(u0);

// Ray payload for primary rays
struct RayPayload
{
    float3 color;
    float3 reflectionColor;
    bool hitGround;
    int depth;
};

// Ray payload for reflection rays
struct ReflectionPayload
{
    float3 color;
    bool hit;
};

// Ray generation shader
[shader("raygeneration")]
void RayGenMain()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;
    
    // Calculate normalized device coordinates
    float2 ndc = (float2(pixelCoord) + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    
    // Generate camera ray
    float4 worldPos = mul(float4(ndc, 0.0f, 1.0f), viewProjInverse);
    worldPos /= worldPos.w;
    
    float3 rayDir = normalize(worldPos.xyz - cameraPosition);
    
    // Setup ray descriptor
    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = rayDir;
    ray.TMin = 0.01f;
    ray.TMax = 100.0f;
    
    // Initialize payload
    RayPayload payload;
    payload.color = float3(0.8f, 0.8f, 0.8f); // Background color
    payload.reflectionColor = float3(0.0f, 0.0f, 0.0f);
    payload.hitGround = false;
    payload.depth = 0;
    
    // Trace primary ray
    TraceRay(g_scene,
             RAY_FLAG_NONE,
             0xFF, // Instance mask
             0, // Hit group index
             1, // Multiplier for geometry contribution to hit group index
             0, // Miss shader index
             ray,
             payload);
    
    // Write result to output texture
    g_output[pixelCoord] = float4(payload.color + payload.reflectionColor, 1.0f);
}

// Miss shader - background
[shader("miss")]
void MissMain(inout RayPayload payload)
{
    // Keep background color (already set in ray gen)
    if (payload.depth == 0)
    {
        // Primary ray miss - keep blue background
        payload.color = float3(0.0f, 0.0f, 1.0f);
    }
    else
    {
        // Reflection ray miss - contribute some ambient
        payload.reflectionColor = float3(0.1f, 0.1f, 0.2f);
    }
}

// Miss shader for reflection rays
[shader("miss")]
void ReflectionMissMain(inout ReflectionPayload payload)
{
    payload.color = float3(0.1f, 0.1f, 0.2f); // Ambient sky color
    payload.hit = false;
}

// Closest hit shader
[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Get hit information
    uint instanceID = InstanceID();
    uint primitiveIndex = PrimitiveIndex();
    float3 barycentrics = float3(1.0f - attribs.barycentrics.x - attribs.barycentrics.y,
                                attribs.barycentrics.x,
                                attribs.barycentrics.y);
    
    float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    float3 normal = float3(0.0f, 1.0f, 0.0f);
    
    if (instanceID == 0) // Ground instance
    {
        payload.hitGround = true;
        
        float3 albedo = float3(0.3f, 0.3f, 0.3f);
        payload.color = albedo;
        
        if (payload.depth < 2)
        {
            // Calculate reflection ray
            float3 reflectDir = reflect(WorldRayDirection(), normal);
            
            RayDesc reflectionRay;
            reflectionRay.Origin = worldPos + normal * 0.01f; // Offset to avoid self-intersection
            reflectionRay.Direction = reflectDir;
            reflectionRay.TMin = 0.01f;
            reflectionRay.TMax = 100.0f;
            
            ReflectionPayload reflPayload;
            reflPayload.color = float3(0.0f, 0.0f, 0.0f);
            reflPayload.hit = false;
            
            // Trace reflection ray
            TraceRay(g_scene,
                     RAY_FLAG_NONE,
                     0xFF,
                     0,
                     1,
                     1, // Use reflection miss shader
                     reflectionRay,
                     reflPayload);
            
            // Add reflection contribution (50% reflection strength)
            payload.reflectionColor = reflPayload.color * 0.5f;
        }
    }
    else // Player or other objects
    {
        // Player gets emissive glow
        float3 emissive = float3(1.0f, 0.8f, 0.4f); // Warm orange glow
        float emissiveStrength = 2.0f;
        
        payload.color = emissive * emissiveStrength;
        payload.reflectionColor = float3(0.0f, 0.0f, 0.0f); // No reflection for player
    }
}

// Closest hit for reflection rays
[shader("closesthit")]
void ReflectionClosestHitMain(inout ReflectionPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint instanceID = InstanceID();
    
    if (instanceID == 0) // Hit ground
    {
        payload.color = float3(0.3f, 0.3f, 0.3f);
    }
    else // Hit player in reflection
    {
        // Player emissive reflection
        float3 emissive = float3(1.0f, 0.8f, 0.4f);
        payload.color = emissive * 1.5f;
    }
    
    payload.hit = true;
}