#define HLSL
#include "RaytracingCommon.h"
#include "RaytracingMaterialEmission.hlsli"
#include "RaytracingTerrain.hlsli"
#include "RaytracingEnvironment.hlsli"
#include "RaytracingNormal.hlsli"

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);

cbuffer CameraConstants : register(b0, space0)
{
	float4x4 g_viewProjInverse;
};

// Bindless Base Buffers
StructuredBuffer<InstanceData> g_instanceBuffer : register(t1, space0);
StructuredBuffer<MaterialGPUData> g_materials : register(t2, space0);
StructuredBuffer<GeoInfo> g_geoTable : register(t3, space0);
StructuredBuffer<TerrainSurfaceGPUData> g_terrainSurfaces : register(t4, space0);

cbuffer RaytracingFrameConstants : register(b2, space0)
{
	uint g_frameSeed;
	uint g_emissionViewMode;
	uint g_environmentMode;
	uint g_frameConstantsPad0;
};

SamplerState g_sampler : register(s0, space0);

static const uint MAX_RECURSION_DEPTH = 3;
static const uint INVALID_TEXTURE_INDEX = 0xffffffffu;

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
	float r = sqrt(RandomValue(seed));
	return circle * r;
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

void BuildOrthonormalBasis(float3 normal, out float3 tangent, out float3 bitangent)
{
	float sign = normal.z >= 0.0f ? 1.0f : -1.0f;
	float a = -1.0f / (sign + normal.z);
	float b2 = normal.x * normal.y * a;
	tangent = float3(1.0f + sign * normal.x * normal.x * a, sign * b2, -sign * normal.x);
	bitangent = float3(b2, sign + normal.y * normal.y * a, -normal.y);
}

float3 SampleDirectionalAreaSun(float3 lightDir, float angularRadius, inout uint state)
{
	float2 disk = RandomPointInCircle(state); // [-1,1] 디스크
	float3 t, b;
	BuildOrthonormalBasis(normalize(lightDir), t, b);
	
	return normalize(lightDir
					+ disk.x * angularRadius * t
					+ disk.y * angularRadius * b
	);
}

inline float ShadowVisibility(
	RaytracingAccelerationStructure accel,
	float3 origin, float3 dir,
	float tMin, float tMax,
	uint mask)
{
	// 플래그: 첫 히트만, 오파큐 가정, 프로시저럴 스킵, 백페이스 컬링 비활성화
	RayQuery <
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
		RAY_FLAG_FORCE_OPAQUE |
		RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
	> rq;
	
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = dir;
	ray.TMin = tMin;
	ray.TMax = tMax;
	
	rq.TraceRayInline(
		accel,
		RAY_FLAG_NONE, // 위 템플릿 파라미터에 이미 플래그 명시
		mask,
		ray
	);

	// 투명물 없음 가정. 후보가 비오파큐면 명시적으로 커밋.
	// 모든 지오메트리가 오파큐면 루프는 거의 한 번만 돎.
	while (rq.Proceed())
	{
		if (rq.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
		{
			rq.CommitNonOpaqueTriangleHit();
		}
	}
	
	return (rq.CommittedStatus() == COMMITTED_NOTHING) ? 1.0f : 0.0f;
}

float SoftShadowVisibilityDirLight(
	RaytracingAccelerationStructure accel,
	float3 origin,
	float3 normal,
	float3 lightDir,
	float angularRadius,
	uint mask,
	inout uint seed)
{
	const uint NUM_SAMPLES = 8;
	float visibility = 0.0f;
	
	[unroll]
	for (uint i = 0; i < NUM_SAMPLES; ++i)
	{
		float3 jitteredDir = SampleDirectionalAreaSun(lightDir, angularRadius, seed);
		
		if (dot(jitteredDir, normal) <= 0.0f)
			continue;

		float v = ShadowVisibility(
			accel,
			origin,
			jitteredDir,
			RAY_TMIN,
			RAY_TMAX,
			mask
		);

		visibility += v;
	}

	return (NUM_SAMPLES > 0) ? visibility / NUM_SAMPLES : 0.0f;
}

#define RAYTRACING_ENABLE_GGX_SAMPLING 1
#include "RaytracingLighting.hlsli"

[shader("raygeneration")]
void RayGenMain()
{
	uint2 pixelCoord = DispatchRaysIndex().xy;
	uint2 screenSize = DispatchRaysDimensions().xy;
	uint pixelIndex = pixelCoord.y * screenSize.x + pixelCoord.x;
	uint rngSeed = pixelIndex * 9781u ^ pixelCoord.y * 6271u ^ screenSize.x * 7919u + 124623u;
		
	float2 jit = RandomPointInCircle(rngSeed) * 0.5f;
	float2 ndc = (float2(pixelCoord) + jit + 0.5f) / float2(screenSize) * 2.0f - 1.0f;
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
	RayPayload payload = MakeDefaultRayPayload(0);
	
	TraceRay(
		g_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,
		0, // HitGroup Index
		0,
		0, // MissShader Index
		ray,
		payload
	);
	
	g_output[pixelCoord] = float4(max(0.0f.xxx, payload.color), 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 lightDir = GetEnvironmentSunDirection();
    // Temporary explicit environment sun NEE.
    float3 lightRadiance = GetEnvironmentSunRadiance(g_environmentMode);
	
	InstanceData inst = g_instanceBuffer[InstanceID()];
	GeoInfo geo = g_geoTable[inst.geoInfoBaseIdx + GeometryIndex()];
	MaterialGPUData mat = g_materials[geo.materialIdx];

	StructuredBuffer<Vertex> vBuffer = ResourceDescriptorHeap[inst.vertexBufferIdx];
	Buffer<uint> iBuffer = ResourceDescriptorHeap[inst.indexBufferIdx];

	float3 bary;
	bary.x = 1.0f - attribs.barycentrics.x - attribs.barycentrics.y;
	bary.y = attribs.barycentrics.x;
	bary.z = attribs.barycentrics.y;
	
	uint triIdx = PrimitiveIndex();
	uint i0 = iBuffer[geo.indexBase + triIdx * 3 + 0];
	uint i1 = iBuffer[geo.indexBase + triIdx * 3 + 1];
	uint i2 = iBuffer[geo.indexBase + triIdx * 3 + 2];

	Vertex v0 = vBuffer[i0];
	Vertex v1 = vBuffer[i1];
	Vertex v2 = vBuffer[i2];
	
	float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	
	float3 normalObj = SafeNormalizeRay(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z, float3(0.0f, 1.0f, 0.0f));
	float3 normal = SafeNormalizeRay(mul(normalObj, (float3x3) inst.worldInverse), float3(0.0f, 1.0f, 0.0f));
	float3 geometricNormal = normal;
	float4 tangentPacked = v0.tangent * bary.x + v1.tangent * bary.y + v2.tangent * bary.z;
	float3 tangentObj = tangentPacked.xyz;
	float tangentSign = tangentPacked.w;
	float3 tangentCandidate = mul(tangentObj, (float3x3) inst.worldMatrix);
	float3 tangent;
	float3 bitangent;
	BuildSafeTangentFrame(normal, tangentCandidate, tangentSign, tangent, bitangent);
	
	float2 uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;
	TerrainSample terrainSample;
	bool useTerrainSplat = 0 != (mat.materialFlags & MATERIAL_FLAG_TERRAIN_SPLAT);
	if (useTerrainSplat)
	{
		terrainSample = SampleTerrainSplat(mat, uv, g_terrainSurfaces, g_sampler);
		normal = TangentNormalToWorld(terrainSample.normalTS, tangent, bitangent, normal);
	}
	else if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_NORMAL_MAP))
	{
		Texture2D normalTexture = ResourceDescriptorHeap[mat.normalTextureIdx];
		float2 encodedXY = normalTexture.SampleLevel(g_sampler, uv, 0).xy * 2.0f - 1.0f;
		float3 normalSample;
		normalSample.xy = encodedXY;
		normalSample.z = sqrt(saturate(1.0f - dot(encodedXY, encodedXY)));
		normal = TangentNormalToWorld(normalSample, tangent, bitangent, normal);
	}
	if (dot(normal, geometricNormal) < 0.0f)
	{
		normal = geometricNormal;
	}
	if (dot(normal, WorldRayDirection()) > 0.0f)
	{
		normal = -normal;
	}

	float3 albedo = mat.albedo.rgb;
	if (useTerrainSplat)
	{
		albedo *= terrainSample.albedo;
	}
	else if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ALBEDO_MAP))
	{
		Texture2D albedoTexture = ResourceDescriptorHeap[mat.albedoTextureIdx];
		albedo *= albedoTexture.SampleLevel(g_sampler, uv, 0).rgb;
	}

	float metallic = mat.metallic;
	float roughness = mat.roughness;
	float ao = 1.0f;
	if (useTerrainSplat)
	{
		metallic = terrainSample.metallic;
		roughness = terrainSample.roughness;
		ao = terrainSample.ao;
	}
	else if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ORM_MAP))
	{
		Texture2D ormTexture = ResourceDescriptorHeap[mat.ormTextureIdx];
		float4 p = ormTexture.SampleLevel(g_sampler, uv, 0);
		if (0 != (mat.materialFlags & MATERIAL_FLAG_UNITY_PACKING))
		{
			metallic = p.r;
			ao = p.g;
			roughness = 1.0 - p.a;
		}
		else
		{
			ao = p.r;
			metallic = p.b;
			roughness = p.g;
		}
	}
	roughness = max(roughness, 0.04);
	
	float3 V = -normalize(WorldRayDirection());
	float NdotVGeom = saturate(dot(geometricNormal, V));
	float NdotVShading = saturate(dot(normal, V));
	float3 H = normalize(V + lightDir);
	float3 F0 = lerp(0.04f.xxx, albedo, metallic);
	
	float NdotL = saturate(dot(normal, lightDir));
	
	float NDF = DistributionGGX(normal, H, roughness);
	float G = GeometrySmith(normal, V, lightDir, roughness);
	float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotL * NdotVShading;
    float3 specular = numerator / max(denominator, EPSILON);
    float3 kD = (1.0 - F) * (1.0 - metallic);
    float3 Lo = (kD * albedo / PI + specular) * lightRadiance * NdotL;
	
	uint rngSeed =
		InstanceID() * 0xc2b2ae35u ^
		PrimitiveIndex() * 0x85ebca6bu ^
		payload.recursionDepth * 0x27d4eb2du ^
		(asuint(hitPos.x) + asuint(hitPos.y) * 0x9e3779b9u + asuint(hitPos.z));
	rngSeed ^= rngSeed >> 16;
	rngSeed *= 0x7feb352d;
	rngSeed ^= rngSeed >> 15;
	rngSeed *= 0x846ca68b;
	rngSeed ^= rngSeed >> 16;
	
    float3 shadowOrigin = hitPos + geometricNormal * 0.1f;
	
	float visibility = SoftShadowVisibilityDirLight(
		g_scene,
		shadowOrigin,
		geometricNormal,
		lightDir,
		GetEnvironmentSunAngularRadius(g_environmentMode),
		0xFF,
		rngSeed
	);
	Lo *= visibility;
	
	float3 reflectedColor = 0.0f.xxx;
	if (payload.recursionDepth < MAX_RECURSION_DEPTH)
	{
		float3 reflectDir;
		if (roughness < 0.15f)
		{
			reflectDir = reflect(WorldRayDirection(), geometricNormal);
		}
		else
		{
			reflectDir = SampleGGXReflection(V, geometricNormal, roughness, rngSeed);
		}
		
		RayDesc reflectRay;
		reflectRay.Origin = hitPos + geometricNormal * 0.1f;
		reflectRay.Direction = reflectDir;
		reflectRay.TMin = RAY_TMIN;
		reflectRay.TMax = RAY_TMAX;
		
		RayPayload reflectPayload = MakeDefaultRayPayload(payload.recursionDepth + 1);
		
		TraceRay(
			g_scene,
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
			0xFF,
			0,
			0,
			0,
			reflectRay,
			reflectPayload
		);
		
		float3 reflectF = FresnelSchlick(NdotVGeom, F0);
		reflectedColor = reflectPayload.color * reflectF * (1.0f - roughness) * 0.5f;
	}
	
    float3 emissive = (0 == payload.recursionDepth)
        ? EvaluateCameraVisibleMaterialEmission(mat, uv, g_emissionViewMode, g_sampler)
        : EvaluateMaterialEmission(mat, uv, g_sampler);
	
    payload.color = Lo + reflectedColor + emissive;
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
	payload.color = SampleEnvironment(WorldRayDirection(), g_environmentMode);
}
