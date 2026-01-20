#define HLSL

// Slot 0 (Table):      
//                      TLAS (t0)
// Slot 1 (Table):      
//                      Output UAV (u0)
// Slot 2 (Constant):   Camera Constants (b0, inline 16 DWORD)
// Slot 3 (Table):       
//                      Materials           (t1)
//                      Vertex Buffer       (t2)
//                      Index Buffer        (t3)
//                      GeoInfo Buffer      (t4)
//                      GeoInstBase Buffer  (t5)

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);

// Camera Constants (16 DWORD = 64 bytes)
cbuffer CameraConstants : register(b0, space0)
{
    float4x4 g_viewProjInverse;
};

struct Material
{
    float3 albedo;
    float metallic;
    float roughness;
    float3 pad_0;
    float3 emissive;
    float emissiveStrength;
};

struct VertexPNU
{
    float3 position;
    float pad_0;
    float3 normal;
    float pad_1;
    float2 uv;
    float2 pad_2;
};

struct GeoInfo
{
    uint vertexBase;
    uint indexBase;
    uint vertexCount;
    uint indexCount;
};

StructuredBuffer<Material> g_materials : register(t1, space0);
StructuredBuffer<VertexPNU> g_vertices : register(t2, space0);
Buffer<uint> g_indices : register(t3, space0);
StructuredBuffer<GeoInfo> g_geoTable : register(t4, space0);
Buffer<uint> g_instGeoBase : register(t5, space0);

struct RayPayload
{
    float3 color;
};

static const float PI = 3.14159265359f;
static const float EPSILON = 0.001f;
static const float RAY_TMIN = 0.001f;
static const float RAY_TMAX = 10000.0f;

inline float HardShadow(float3 origin, float3 dir)
{
    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE > rq;
    
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = RAY_TMIN;
    ray.TMax = RAY_TMAX;
    
    rq.TraceRayInline(g_scene, RAY_FLAG_NONE, 0xFF, ray);
    rq.Proceed();
    
    return (rq.CommittedStatus() == COMMITTED_NOTHING) ? 1.0f : 0.0f;
}

float3 SimpleSky(float3 rayDir)
{
    float t = rayDir.y * 0.5f + 0.5f;
    return lerp(float3(0.8, 0.9, 1.0), float3(0.3, 0.5, 0.8), t);
}

[shader("raygeneration")]
void RayGenMain()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;
    
    float2 ndc = (float2(pixelCoord) + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    
    float4 worldPos = mul(float4(ndc, 1.0f, 1.0f), g_viewProjInverse);
    worldPos /= worldPos.w;
    float4 cameraPos = mul(float4(0, 0, 0, 1), g_viewProjInverse);
    cameraPos /= cameraPos.w;
    
    RayDesc ray;
    ray.Origin = cameraPos.xyz;
    ray.Direction = normalize(worldPos.xyz - cameraPos.xyz);
    ray.TMin = RAY_TMIN;
    ray.TMax = RAY_TMAX;
    
    RayPayload payload;
    payload.color = float3(0, 0, 0);
    
    TraceRay(
        g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF,
        0,
        0,
        0,
        ray,
        payload
    );
    
    float3 finalColor = pow(payload.color, 1.0f / 2.2f);
    g_output[pixelCoord] = float4(finalColor, 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float3 lightColor = float3(1.0, 1.0, 1.0) * 2.0;
    
    uint instanceID = InstanceID();
    Material material = g_materials[instanceID];
    uint geoBase = g_instGeoBase[instanceID];
    
    float3 albedo = material.albedo;
    float metallic = material.metallic;
    float roughness = max(material.roughness, 0.1);
    
    float3 bary;
    bary.x = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
    bary.y = attribs.barycentrics.x;
    bary.z = attribs.barycentrics.y;
    
    uint geoIdx = geoBase + GeometryIndex();
    GeoInfo gi = g_geoTable[geoIdx];

    uint tri = PrimitiveIndex();
    uint i0 = g_indices[gi.indexBase + tri * 3 + 0];
    uint i1 = g_indices[gi.indexBase + tri * 3 + 1];
    uint i2 = g_indices[gi.indexBase + tri * 3 + 2];

    VertexPNU v0 = g_vertices[gi.vertexBase + i0];
    VertexPNU v1 = g_vertices[gi.vertexBase + i1];
    VertexPNU v2 = g_vertices[gi.vertexBase + i2];
    
    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    float3 normalObj = normalize(
        v0.normal * bary.x +
        v1.normal * bary.y +
        v2.normal * bary.z
    );
    float3x3 objectToWorld = (float3x3) ObjectToWorld3x4();
    float3 normal = normalize(mul(normalObj, transpose(objectToWorld)));
    if (dot(normal, WorldRayDirection()) > 0.0f)
        normal = -normal;
    
    float NdotL = saturate(dot(normal, lightDir));
    
    float3 diffuse = albedo * NdotL;
    
    float3 specular = 0.0f.xxx;
    if (metallic > 0.5f)
    {
        float3 V = -normalize(WorldRayDirection());
        float3 H = normalize(V + lightDir);
        float NdotH = saturate(dot(normal, H));
        float shininess = (1.0f - roughness) * 128.0f + 2.0f;
        specular = pow(NdotH, shininess) * metallic * lightColor;
    }
    
    float3 shadowOrigin = hitPos + normal * 0.01f;
    float shadow = HardShadow(shadowOrigin, lightDir);
    
    float3 directLight = (diffuse + specular) * lightColor * shadow;
    
    float3 ambient = albedo * 0.15f;
    
    float3 emissive = material.emissive * material.emissiveStrength;
    
    payload.color = ambient + directLight + emissive;
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    float3 rayDir = WorldRayDirection();
    payload.color = SimpleSky(rayDir);
}