#define HLSL
#include "RaytracingCommon.hlsli"

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);

cbuffer CameraConstants : register(b0, space0)
{
	float4x4 g_viewProjInverse;
};

StructuredBuffer<InstanceData> g_instanceBuffer : register(t1, space0);
StructuredBuffer<MaterialGPUData> g_materials : register(t2, space0);
StructuredBuffer<GeoInfo> g_geoTable : register(t3, space0);

SamplerState g_sampler : register(s0, space0);

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
	float r = sqrt(RandomValue(seed));
	return float2(cos(angle), sin(angle)) * r;
}
float3 CosineHemisphere(float3 normal, inout uint seed)
{
	float u1 = RandomValue(seed);
	float u2 = RandomValue(seed);
	float r = sqrt(u1);
	float theta = 2.0f * PI * u2;
	float x = r * cos(theta);
	float z = r * sin(theta);
	float y = sqrt(max(0.0f, 1.0f - u1));
	float3 w = normal;
	float3 u = normalize(cross(abs(w.x) > 0.1f ? float3(0, 1, 0) : float3(1, 0, 0), w));
	float3 v = cross(w, u);
	return normalize(u * x + v * z + w * y);
}
float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
	return a2 / max(PI * denom * denom, EPSILON);
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;
	return NdotV / max(NdotV * (1.0f - k) + k, EPSILON);
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	return GeometrySchlickGGX(max(dot(N, V), 0.0f), roughness) * GeometrySchlickGGX(max(dot(N, L), 0.0f), roughness);
}
float3 SampleGGX(float3 N, float3 V, float roughness, inout uint seed)
{
	float a = roughness * roughness;
	float u1 = RandomValue(seed);
	float u2 = RandomValue(seed);
	float theta = atan(a * sqrt(u1) / sqrt(1.0f - u1));
	float phi = 2.0f * PI * u2;
	float3 H_local = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
	float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	return reflect(-V, normalize(tangent * H_local.x + N * H_local.y + bitangent * H_local.z));
}
float GGX_PDF(float3 N, float3 H, float3 V, float3 L, float roughness)
{
	float NdotH = max(dot(N, H), 0.0f);
	float VdotH = max(dot(V, H), 0.0f);
	float D = DistributionGGX(N, H, roughness);
	return (D * NdotH) / max(4.0f * VdotH, EPSILON);
}

[shader("raygeneration")]
void RayGenMain()
{
	uint2 pixelCoord = DispatchRaysIndex().xy;
	uint2 screenSize = DispatchRaysDimensions().xy;
	uint pixelIndex = pixelCoord.y * screenSize.x + pixelCoord.x;
	uint rngSeed = pixelIndex * 9781u ^ pixelCoord.y * 6271u ^ screenSize.x * 7919u + 124623u;
	float3 finalColor = 0.0f.xxx;
	for (uint bounce = 0; bounce < 4; ++bounce)
	{
		float2 jit = RandomPointInCircle(rngSeed) * 0.5f;
		float2 ndc = (float2(pixelCoord) + jit + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
		ndc.y = -ndc.y;
		float4 worldPos = mul(float4(ndc, 1.0f, 1.0f), g_viewProjInverse);
		worldPos /= worldPos.w;
		float4 cameraPos = mul(float4(0, 0, 0, 1), g_viewProjInverse);
		cameraPos /= cameraPos.w;
		RayDesc ray = { cameraPos.xyz, 0.001, normalize(worldPos.xyz - cameraPos.xyz), 1000.0 };
		RayPayload payload = { 0.0f.xxx, 0 };
		TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
		finalColor += payload.color;
	}
	g_output[pixelCoord] = float4(pow(finalColor / 4.0, (1.0f / 2.2f).xxx), 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	InstanceData inst = g_instanceBuffer[InstanceID()];
	GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + GeometryIndex()];
	MaterialGPUData mat = g_materials[geo.materialIdx];

	StructuredBuffer<Vertex> vBuffer = ResourceDescriptorHeap[inst.vertexBufferIdx];
	Buffer<uint> iBuffer = ResourceDescriptorHeap[inst.indexBufferIdx];

	uint triIdx = PrimitiveIndex();
	uint i0 = iBuffer[geo.indexBase + triIdx * 3 + 0];
	uint i1 = iBuffer[geo.indexBase + triIdx * 3 + 1];
	uint i2 = iBuffer[geo.indexBase + triIdx * 3 + 2];

	Vertex v0 = vBuffer[i0];
	Vertex v1 = vBuffer[i1];
	Vertex v2 = vBuffer[i2];
	float3 bary = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	float3 normal = normalize(mul(normalize(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z), (float3x3) inst.worldIT));
	if (dot(normal, WorldRayDirection()) > 0.0f)
		normal = -normal;
	float2 uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;

	float3 albedo = mat.albedo.rgb;

	if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ALBEDO_MAP))
	{
		Texture2D albedoTex = ResourceDescriptorHeap[mat.albedoTextureIdx];
		albedo *= albedoTex.SampleLevel(g_sampler, uv, 0).rgb;
	}
	
	float metal = mat.metallic;
	float rough = mat.roughness;

	if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ORM_MAP))
	{
		Texture2D ormTex = ResourceDescriptorHeap[mat.ormTextureIdx];
		float4 p = ormTex.SampleLevel(g_sampler, uv, 0);
		if (0 != (mat.materialFlags & MATERIAL_FLAG_UNITY_PACKING))
		{
			metal = p.r;
			rough = 1.0 - p.a;
		}
		else
		{
			metal = p.b;
			rough = p.g;
		}
	}
	rough = max(rough, 0.04);

	float3 V = -normalize(WorldRayDirection());
	float3 F0 = lerp(0.04f.xxx, albedo, metal);
	if (payload.recursionDepth >= 18)
	{
		payload.color = float3(0, 0, 0);
		return;
	}
	
	uint seed = InstanceID() ^ PrimitiveIndex() ^ payload.recursionDepth;
	float3 indirectF = FresnelSchlick(max(dot(normal, V), 0.0f), F0);
	float specProb = clamp(dot(indirectF, 0.333f.xxx), 0.01, 0.99);
	
	float3 L, weight;
	float pdf;
	if (RandomValue(seed) > specProb)
	{
		L = CosineHemisphere(normal, seed);
		float NdotL = max(dot(normal, L), 0.0f);
		weight = (1.0f.xxx - indirectF) * (1.0 - metal) * albedo; // Simplified PT throughput
	}
	else
	{
		L = SampleGGX(normal, V, rough, seed);
		weight = indirectF; // Simplified
	}
	
	RayDesc nextRay = { hitPos + normal * 0.01, 0.001, normalize(L), 1000.0 };
	RayPayload child = { 0.0f.xxx, payload.recursionDepth + 1 };
	TraceRay(g_scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, nextRay, child);
	payload.color = weight * child.color;
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
	payload.color = lerp(float3(0.1, 0.2, 0.4), float3(0.5, 0.7, 1.0), WorldRayDirection().y * 0.5 + 0.5);
}
