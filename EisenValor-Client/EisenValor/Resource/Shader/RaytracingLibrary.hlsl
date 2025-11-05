#define HLSL
#include "RaytracingCommon.hlsli"

// Global Root Signature
// Slot 0: Acceleration Structure (TLAS, Vertex, VertexOffset, Material)
// Slot 1: Output UAV 
// Slot 2: Camera Constants
// Global Root Signature 정의와 일치
// Slot 0: TLAS (t0)
// Slot 1: Output UAV (u0)
// Slot 2: Camera Constants (b0, inline 16 DWORD)
// Slot 3: Materials (t1)

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);

struct Material
{
    float3 albedo;
    float metallic;
    float roughness;
    float3 emissive;
    float emissiveStrength;
};

StructuredBuffer<Material> g_materials : register(t1, space0);
// Camera Constants (16 DWORD = 64 bytes)
cbuffer CameraConstants : register(b0, space0)
{
    float4x4 g_viewProjInverse;
};

struct RayPayload
{
    float3 color;
};

// 프리넬 근사
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// GGX Normal Distribution Function
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265359f * denom * denom;
    
    return nom / max(denom, 0.0000001f);
}

// Geometry Function - Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return nom / max(denom, 0.0000001f);
}

// GGX
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
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
    
    float3 cameraPos = float3(g_viewProjInverse[3][0], g_viewProjInverse[3][1], g_viewProjInverse[3][2]);
    
    RayDesc ray;
    ray.Origin = cameraPos;
    ray.Direction = normalize(worldPos.xyz - cameraPos);
    ray.TMin = 0.001f;
    ray.TMax = 1000.0f;
    
    RayPayload payload;
    payload.color = float3(0, 0, 0);
    
    TraceRay(
        g_scene,
        RAY_FLAG_NONE,
        0xFF,
        0, // HitGroup Index
        0,
        0, // MissShader Index
        ray,
        payload
    );
    
    float3 finalColor = payload.color;
    g_output[pixelCoord] = float4(finalColor, 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint instanceID = InstanceID();
    Material material = g_materials[instanceID];
    
    float3 barycentrics;
    barycentrics.x = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
    barycentrics.y = attribs.barycentrics.x;
    barycentrics.z = attribs.barycentrics.y;
    
    float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    // TODO: VertexBuffer 등록후 보간값 사용
    float3 normal = float3(0, 1, 0);
    
    if (instanceID == 0)
    {
        normal = float3(0, 1, 0);
    }
    else
    {
        normal = normalize(float3(
            barycentrics.x - 0.333f,
            barycentrics.y - 0.333f,
            barycentrics.z - 0.333f
        ));
        
        float3 rayDir = WorldRayDirection();
        if (dot(normal, rayDir) > 0.0f)
            normal = -normal;
    }
    
    float3 V = -normalize(WorldRayDirection());
    
    float3 L = normalize(float3(0.5f, 1.0f, 0.3f));
    float3 H = normalize(V + L);
    float3 radiance = float3(1.0f, 0.95f, 0.8f) * 3.0f;
    
    float3 albedo = material.albedo;
    float metallic = material.metallic;
    float roughness = max(material.roughness, 0.04f);
    
    // zero incidence
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(normal, V), 0.0f) * max(dot(normal, L), 0.0f);
    float3 specular = numerator / max(denominator, 0.001f);
    
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - metallic;
    
    float NdotL = max(dot(normal, L), 0.0f);
    float3 Lo = (kD * albedo / 3.14159265359f + specular) * radiance * NdotL;
    
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * (1.0f + metallic * 0.5f);
    
    float ao = barycentrics.x * 0.3f + 0.7f;
    
    float3 emissive = material.emissive * material.emissiveStrength;
    
    float3 color = ambient * ao + Lo + emissive;
    
    payload.color = color;
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    payload.color = 0.0f.xxx;
  //  float3 rayDir = WorldRayDirection();
  //  float t = 0.5f * (rayDir.y + 1.0f);
  //  payload.color = lerp(float3(1, 1, 1), float3(0.5f, 0.7f, 1.0f), t);
}

/*
// =======================================================================================
// EisenValor Raytracing Library
// Ground reflection + Player emissive raytracing
// =======================================================================================

#define HLSL
#include "RaytracingCommon.hlsli"

// Global Root Signature
// Slot 0: Acceleration Structure (TLAS, Vertex, VertexOffset, Material)
// Slot 1: Output UAV 
// Slot 2: Camera Constants

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

struct MaterialConstants
{
	float3 albedo;
	float metallic;
	float roughness;
	float emissiveStrength;
	float3 emissive;
	uint flags;
	uint _padding[2];
};

struct Vertex
{
	float3 position;
	float3 normal;
	float4 color;
};

RaytracingAccelerationStructure g_scene : register(t0);
StructuredBuffer<Vertex> g_vertices : register(t1);
StructuredBuffer<uint> g_indices : register(t2);
StructuredBuffer<uint> g_vertexOffsets : register(t3);
StructuredBuffer<uint> g_indexOffsets : register(t4);
StructuredBuffer<MaterialConstants> g_materials : register(t5);
RWTexture2D<float4> g_output : register(u0);

struct RayPayload
{
	float3 color;
	uint recursionDepth;
};

[shader("raygeneration")]
void RayGenMain()
{
	uint2 pixelCoord = DispatchRaysIndex().xy;
	uint2 screenSize = DispatchRaysDimensions().xy;
	
	float2 ndc = (float2(pixelCoord) + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
	ndc.y = -ndc.y;
	float4 worldPos = mul(float4(ndc, 1.0f, 1.0f), viewProjInverse);
	worldPos /= worldPos.w;
	
	RayDesc ray;
	ray.Origin = cameraPosition;
	ray.Direction = normalize(worldPos.xyz - cameraPosition);
	ray.TMin = 0.001f;
	ray.TMax = 1000.0f;

	RayPayload payload;
	payload.color = 0.0f.xxx;
	payload.recursionDepth = 0;

	TraceRay(g_scene,
			 RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
			 0xFF,
			 0, 0, 0,
			 ray,
			 payload);
	float3 finalColor = payload.color.rgb;
	finalColor *= 1.2f;
	
	// ACES 톤매핑 근사
	float3 a = finalColor * 2.51f;
	float3 b = finalColor * 0.03f + 0.59f;
	float3 c = finalColor * 2.43f + 0.14f;
	finalColor = saturate((a) / (b + c));
	
	// 감마 보정
	finalColor = pow(finalColor, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));
	
	g_output[pixelCoord.xy] = float4(finalColor, 1.0f);
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
	float3 rayDir = WorldRayDirection();
	float t = 0.5f * (rayDir.y + 1.0f);
	float starField = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		float2 starCoord = rayDir.xz * (10.0f + float(i) * 5.0f) + float(i) * 100.0f;
		float star = pow(max(0.0f,
			sin(starCoord.x * 20.0f) *
			cos(starCoord.y * 15.0f)), 20.0f);
		starField += star * (1.0f - t) * 0.3f;
	}
	
	float3 stars = float3(1, 1, 1) * starField;
	payload.color += 0.05f.xxx + stars;
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	uint instanceID = InstanceID();
	const uint primIndex = PrimitiveIndex();
	const uint vertexOffset = g_vertexOffsets[instanceID];
	const uint indexOffset = g_indexOffsets[instanceID];
	const uint3 indices = uint3(g_indices[indexOffset + 3 * primIndex + 0],
									g_indices[indexOffset + 3 * primIndex + 1],
									g_indices[indexOffset + 3 * primIndex + 2]
	);
	
	const Vertex v0 = g_vertices[vertexOffset + indices.x];
	const Vertex v1 = g_vertices[vertexOffset + indices.y];
	const Vertex v2 = g_vertices[vertexOffset + indices.z];
	const MaterialConstants material = g_materials[instanceID];
	float3 barycentrics;
	barycentrics.yz = attribs.barycentrics.xy;
	barycentrics.x = 1.0f - barycentrics.y - barycentrics.z;

	float3 normal = normalize(barycentrics.x * v0.normal +
	barycentrics.y * v1.normal + barycentrics.z * v2.normal);

	float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	float3 viewDir = -normalize(WorldRayDirection());
	float3 baseColor = material.albedo;
	float metallic = material.metallic;
	float roughness = material.roughness;
	
	float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
	float3 lightColor = float3(1.0f, 0.95f, 0.8f) * 2.0f;
	
	float NdotL = max(0.0f, dot(normal, lightDir));
	float3 diffuse = baseColor * lightColor * NdotL;
	
	float3 ambient = baseColor * float3(0.1f, 0.15f, 0.2f);
	float3 reflectedColor = float3(0, 0, 0);
	
	float3 specular = float3(0, 0, 0);
	float3 halfVector = normalize(lightDir + viewDir);
	float NdotH = max(0.0f, dot(normal, halfVector));
	float specPower = lerp(128.0f, 8.0f, roughness);
	specular = lightColor * pow(NdotH, specPower) * (1.0f - roughness) * 0.3f;
	
	if (metallic > 0.1f && payload.recursionDepth < 2)
	{
		float3 reflectDir = reflect(-viewDir, normal);
		
		RayDesc reflectRay;
		reflectRay.Origin = worldPos + normal * 0.001f;
		reflectRay.Direction = reflectDir;
		reflectRay.TMin = 0.001f;
		reflectRay.TMax = 1000.0f;
		
		RayPayload reflectPayload;
		reflectPayload.color = 0.0f.xxx;
		reflectPayload.recursionDepth = payload.recursionDepth + 1;
		
		TraceRay(g_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,
		0, 0, 0,
		reflectRay,
		reflectPayload);
		
		reflectedColor = reflectPayload.color.rgb * metallic * (1.0f - roughness);
	}
	float fresnel = pow(1.0f - max(0.0f, dot(viewDir, normal)), 3.0f);
	float3 fresnelColor = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
	
	float3 dielectric = diffuse + specular * fresnelColor;
	float3 conductor = baseColor * specular + reflectedColor;
	
	
	float3 finalColor = 0.0f.xxx;
	finalColor = lerp(dielectric, conductor, metallic) + ambient;
	float edgeGlow = pow(1.0f - abs(dot(viewDir, normal)), 2.0f);
	finalColor += baseColor * edgeGlow * 0.1f;
	finalColor += material.emissive * material.emissiveStrength;
	payload.color += finalColor;
}
*/