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