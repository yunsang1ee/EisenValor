#define HLSL
#include "RaytracingCommon.h"

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

SamplerState g_sampler : register(s0, space0);

static const uint MAX_RECURSION_DEPTH = 3;

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

float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;
	
	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;

	return nom / max(denom, EPSILON);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;
	
	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / max(denom, EPSILON);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	
	return ggx1 * ggx2;
}

float3 SampleGGXReflection(float3 V, float3 N, float roughness, inout uint seed)
{
	float a = roughness * roughness;
	float u1 = RandomValue(seed);
	float u2 = RandomValue(seed);
	
	float theta = atan(a * sqrt(u1) / sqrt(1.0f - u1 + EPSILON));
	float phi = 2.0f * PI * u2;
	
	float3 H_local;
	H_local.x = sin(theta) * cos(phi);
	H_local.y = cos(theta);
	H_local.z = sin(theta) * sin(phi);
	
	float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 H = normalize(tangent * H_local.x + N * H_local.y + bitangent * H_local.z);
	return reflect(-V, H);
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

// 배경을 위한
float3 AtmosphereGradient(float3 rayDir, float3 sunDir)
{
	float height = rayDir.y;
	float sunHeight = sunDir.y;
  
	float horizonFade = smoothstep(-0.02, 0.05, height);
  
	float3 zenithColor = lerp(float3(0.3, 0.4, 0.6), float3(0.5, 0.7, 1.0), smoothstep(-0.2, 0.5, sunHeight));
	float3 horizonColor = lerp(float3(0.8, 0.4, 0.2), float3(0.9, 0.8, 0.7), smoothstep(-0.2, 0.2, sunHeight));
  
	float3 skyColor = lerp(horizonColor, zenithColor, smoothstep(0.0, 0.4, height));
  
	float sunInfluence = max(0.0, dot(rayDir, sunDir));
	float3 sunTint = float3(1.0, 0.8, 0.6) * pow(sunInfluence, 8.0) * 0.3;
  
	return skyColor * horizonFade + sunTint;
}

float3 RayleighScattering(float3 rayDir, float3 sunDir)
{
	float cosTheta = dot(rayDir, sunDir);
	float rayleighPhase = 3.0 / (16.0 * 3.14159) * (1.0 + cosTheta * cosTheta);
  
	float3 wavelengths = float3(0.65, 0.57, 0.475);
	float3 scatterCoeff = 1.0 / pow(wavelengths, 4.0f.xxx);
  
	return scatterCoeff * rayleighPhase * 0.3;
}

float3 MieScattering(float3 rayDir, float3 sunDir)
{
	float cosTheta = dot(rayDir, sunDir);
	float g = 0.76;
	float g2 = g * g;
  
	float miePhase = (3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2)) *
				   (1.0 + cosTheta * cosTheta) / pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
  
	return 1.0f.xxx * miePhase * 0.1;
}

float3 SunHalo(float3 rayDir, float3 sunDir)
{
	float sunDot = max(0.0, dot(rayDir, sunDir));
  
	float sunCore = pow(sunDot, 2000.0) * 2.0;
  
	float halo1 = pow(sunDot, 200.0) * 0.8;
	float halo2 = pow(sunDot, 50.0) * 0.4;
	float halo3 = pow(sunDot, 20.0) * 0.2;
  
	float corona = pow(sunDot, 10.0) * 0.1;
  
	float3 sunColor = float3(1.0, 0.95, 0.8);
	float3 haloColor1 = float3(1.0, 0.9, 0.7);
	float3 haloColor2 = float3(1.0, 0.8, 0.6);
	float3 coronaColor = float3(0.9, 0.7, 0.5);
  
	return sunCore * sunColor +
		 halo1 * haloColor1 +
		 halo2 * haloColor2 +
		 halo3 * haloColor1 * 0.5 +
		 corona * coronaColor;
}

float CloudNoise(float3 rayDir)
{
	float3 p = rayDir * 5.0;
	return frac(sin(dot(p, float3(12.9898, 78.233, 45.164))) * 43758.5453) * 0.5 + 0.5;
}

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
	RayPayload payload;
	payload.color = 0.0f.xxx;
	payload.recursionDepth = 0;
	
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
	
	float3 finalColor = payload.color;
	
	float3 bloom = max(0.0f.xxx, finalColor - 0.75f.xxx) * 0.7f;
	finalColor += bloom;
	
	finalColor = pow(finalColor, (1.0f / 1.8f).xxx);
	
	float luminance = dot(finalColor, float3(0.299f, 0.587f, 0.114f));
	finalColor = lerp(luminance.xxx, finalColor, 1.4f);
	finalColor += float3(0.05f, 0.03f, 0.01f);
	finalColor = clamp(finalColor, 0.0f, 1.0f);
	
	g_output[pixelCoord] = float4(finalColor, 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
	float3 lightColor = float3(1.0, 1.0, 1.0) * 3.0;
	
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
	
	float3 normalObj = normalize(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z);
	float3 normal = normalize(mul(normalObj, (float3x3) inst.worldInverse));
	if (dot(normal, WorldRayDirection()) > 0.0f)
	{
		normal = -normal;
	}
	
	float2 uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;

	float3 albedo = mat.albedo.rgb;
	if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ALBEDO_MAP))
	{
		Texture2D albedoTexture = ResourceDescriptorHeap[mat.albedoTextureIdx];
		albedo *= albedoTexture.SampleLevel(g_sampler, uv, 0).rgb;
	}

	float metallic = mat.metallic;
	float roughness = mat.roughness;
	if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_ORM_MAP))
	{
		Texture2D ormTexture = ResourceDescriptorHeap[mat.ormTextureIdx];
		float4 p = ormTexture.SampleLevel(g_sampler, uv, 0);
		if (0 != (mat.materialFlags & MATERIAL_FLAG_UNITY_PACKING))
		{
			metallic = p.r;
			roughness = 1.0 - p.a;
		}
		else
		{
			metallic = p.b;
			roughness = p.g;
		}
	}
	roughness = max(roughness, 0.04);
	
	float3 V = -normalize(WorldRayDirection());
	float3 H = normalize(V + lightDir);
	float3 F0 = lerp(0.04f.xxx, albedo, metallic);
	
	float NdotL = saturate(dot(normal, lightDir));
	float NdotV = saturate(dot(normal, V));
	
	float NDF = DistributionGGX(normal, H, roughness);
	float G = GeometrySmith(normal, V, lightDir, roughness);
	float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * NdotL * NdotV;
	float3 specular = numerator / max(denominator, EPSILON);
	float3 kD = (1.0 - F) * (1.0 - metallic);
	float3 Lo = (kD * albedo / PI + specular) * lightColor * NdotL;
	
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
	
	float3 shadowOrigin = hitPos + normal * 0.1f;
	float angularRadius = 0.1f;
	
	float visibility = SoftShadowVisibilityDirLight(
		g_scene,
		shadowOrigin,
		normal,
		lightDir,
		angularRadius,
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
			reflectDir = reflect(WorldRayDirection(), normal);
		}
		else
		{
			reflectDir = SampleGGXReflection(V, normal, roughness, rngSeed);
		}
		
		RayDesc reflectRay;
		reflectRay.Origin = hitPos + normal * 0.1f;
		reflectRay.Direction = reflectDir;
		reflectRay.TMin = RAY_TMIN;
		reflectRay.TMax = RAY_TMAX;
		
		RayPayload reflectPayload;
		reflectPayload.color = 0.0f.xxx;
		reflectPayload.recursionDepth = payload.recursionDepth + 1;
		
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
		
		float3 reflectF = FresnelSchlick(NdotV, F0);
		reflectedColor = reflectPayload.color * reflectF * (1.0f - roughness) * 0.5f;
	}
	
	float ao = bary.x * 0.3f + 0.7f;
	float3 ambient = kD * albedo * 0.15f * ao;
	float3 emissive = 0.0f.xxx; // No emissive for now
	
	payload.color = ambient + Lo + reflectedColor + emissive;
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
	float3 rayDir = WorldRayDirection();
	float3 sunDir = normalize(float3(0.5, 1.0, 0.3));
	
	float3 atmosphere = AtmosphereGradient(rayDir, sunDir);
	atmosphere += RayleighScattering(rayDir, sunDir);
	atmosphere += MieScattering(rayDir, sunDir);
	atmosphere += SunHalo(rayDir, sunDir);
  
	if (rayDir.y > 0.1)
	{
		float clouds = CloudNoise(rayDir);
		clouds = smoothstep(0.6, 0.8, clouds) * 0.3;
		atmosphere = lerp(atmosphere, float3(0.9, 0.9, 0.95), clouds);
	}

	float3 groundColor = lerp(float3(0.3, 0.2, 0.1), float3(0.4, 0.3, 0.2),
						smoothstep(-0.5, 0.0, sunDir.y));
  
	float groundToSkyT = smoothstep(-0.01, 0.0, rayDir.y);
  
	payload.color = lerp(groundColor, atmosphere, groundToSkyT);
}
