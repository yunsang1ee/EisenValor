#define HLSL
#include "RaytracingCommon.h"

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);

cbuffer CameraConstants : register(b0, space0)
{
	float4x4 g_viewProjInverse;
};

StructuredBuffer<InstanceData> g_instanceBuffer : register(t1, space0);
StructuredBuffer<MaterialGPUData> g_materials : register(t2, space0);
StructuredBuffer<GeoInfo> g_geoTable : register(t3, space0);
StructuredBuffer<TerrainSurfaceGPUData> g_terrainSurfaces : register(t4, space0);

SamplerState g_sampler : register(s0, space0);

static const uint MAX_RECURSION_DEPTH = 18;
static const uint SPP = 4;
static const uint INVALID_TEXTURE_INDEX = 0xffffffffu;

struct TerrainSample
{
	float3 albedo;
	float3 normalTS;
	float metallic;
	float roughness;
	float ao;
};

void AccumulateTerrainLayer(
	inout TerrainSample blended,
	inout float weightSum,
	uint albedoTextureIdx,
	uint normalTextureIdx,
	uint ormTextureIdx,
	float2 metallicRoughness,
	float weight,
	float2 terrainXZ,
	float4 tileST)
{
	if (weight <= 0.0f)
	{
		return;
	}

	float2 layerUV = terrainXZ * tileST.xy + tileST.zw;
	float3 layerAlbedo = 1.0f.xxx;
	if (albedoTextureIdx != INVALID_TEXTURE_INDEX)
	{
		Texture2D albedoTexture = ResourceDescriptorHeap[albedoTextureIdx];
		layerAlbedo = albedoTexture.SampleLevel(g_sampler, layerUV, 0).rgb;
	}

	float3 layerNormalTS = float3(0.0f, 0.0f, 1.0f);
	if (normalTextureIdx != INVALID_TEXTURE_INDEX)
	{
		Texture2D normalTexture = ResourceDescriptorHeap[normalTextureIdx];
		float2 encodedXY = normalTexture.SampleLevel(g_sampler, layerUV, 0).xy * 2.0f - 1.0f;
		layerNormalTS = float3(encodedXY, sqrt(saturate(1.0f - dot(encodedXY, encodedXY))));
	}

	float layerMetallic = metallicRoughness.x;
	float layerRoughness = metallicRoughness.y;
	float layerAo = 1.0f;
	if (ormTextureIdx != INVALID_TEXTURE_INDEX)
	{
		Texture2D ormTexture = ResourceDescriptorHeap[ormTextureIdx];
		float4 p = ormTexture.SampleLevel(g_sampler, layerUV, 0);
		layerAo = p.r;
		layerMetallic = p.b;
		layerRoughness = p.g;
	}

	blended.albedo += layerAlbedo * weight;
	blended.normalTS += layerNormalTS * weight;
	blended.metallic += layerMetallic * weight;
	blended.roughness += layerRoughness * weight;
	blended.ao += layerAo * weight;
	weightSum += weight;
}

TerrainSample SampleTerrainSplat(MaterialGPUData mat, float2 terrainUV)
{
	TerrainSample result;
	result.albedo = 1.0f.xxx;
	result.normalTS = float3(0.0f, 0.0f, 1.0f);
	result.metallic = 0.0f;
	result.roughness = 1.0f;
	result.ao = 1.0f;

	if (mat.terrainSurfaceIdx == INVALID_TEXTURE_INDEX)
	{
		return result;
	}

	TerrainSurfaceGPUData terrain = g_terrainSurfaces[mat.terrainSurfaceIdx];
	if (terrain.splatTextureIdx == INVALID_TEXTURE_INDEX)
	{
		return result;
	}

	Texture2D splatTexture = ResourceDescriptorHeap[terrain.splatTextureIdx];
	float4 weights = splatTexture.SampleLevel(g_sampler, terrainUV, 0);
	float2 terrainXZ = terrainUV * terrain.terrainSize.xy;
	uint layerCount = terrain.layerCount;

	TerrainSample blended;
	blended.albedo = 0.0f.xxx;
	blended.normalTS = 0.0f.xxx;
	blended.metallic = 0.0f;
	blended.roughness = 0.0f;
	blended.ao = 0.0f;
	float weightSum = 0.0f;
	if (0 < layerCount)
	{
		AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx0, terrain.layerNormalTextureIdx0, terrain.layerOrmTextureIdx0, terrain.layerMetallicRoughness[0].xy, weights.r, terrainXZ, terrain.layerTileST[0]);
	}
	if (1 < layerCount)
	{
		AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx1, terrain.layerNormalTextureIdx1, terrain.layerOrmTextureIdx1, terrain.layerMetallicRoughness[1].xy, weights.g, terrainXZ, terrain.layerTileST[1]);
	}
	if (2 < layerCount)
	{
		AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx2, terrain.layerNormalTextureIdx2, terrain.layerOrmTextureIdx2, terrain.layerMetallicRoughness[2].xy, weights.b, terrainXZ, terrain.layerTileST[2]);
	}
	if (3 < layerCount)
	{
		AccumulateTerrainLayer(blended, weightSum, terrain.layerAlbedoTextureIdx3, terrain.layerNormalTextureIdx3, terrain.layerOrmTextureIdx3, terrain.layerMetallicRoughness[3].xy, weights.a, terrainXZ, terrain.layerTileST[3]);
	}

	if (0.0f < weightSum)
	{
		result.albedo = blended.albedo / weightSum;
		result.normalTS = normalize(blended.normalTS / weightSum);
		result.metallic = blended.metallic / weightSum;
		result.roughness = blended.roughness / weightSum;
		result.ao = blended.ao / weightSum;
	}
	return result;
}

// RNG (PCG)
float RandomValue(inout uint seed)
{
	seed = seed * 747796405u + 2891336453u;
	uint temp = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
	temp = (temp >> 22u) ^ temp;
	return float(temp) / 4294967296.0f;
}

// 원 내부의 임의 점 생성
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

// 프리넬 근사
float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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
	denom = PI * denom * denom;

	return nom / max(denom, EPSILON);
}

// Geometry Function - Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;
	
	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / max(denom, EPSILON);
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

// GGX Importance Sampling
float3 SampleGGX(float3 N, float3 V, float roughness, inout uint seed)
{
	float a = roughness * roughness;
	
	float u1 = RandomValue(seed);
	float u2 = RandomValue(seed);
	
	float theta = atan(a * sqrt(u1) / sqrt(1.0f - u1));
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

// GGX PDF
float GGX_PDF(float3 N, float3 H, float3 V, float3 L, float roughness)
{
	float NdotH = max(dot(N, H), 0.0f);
	float VdotH = max(dot(V, H), 0.0f);
	
	float D = DistributionGGX(N, H, roughness);
	return (D * NdotH) / max(4.0f * VdotH, EPSILON);
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
	
	uint rngSeed = pixelIndex * 9781u
					 ^ pixelCoord.y * 6271u
					 ^ screenSize.x * 7919u + 124623u;
	
	float3 finalColor = 0.0f.xxx;
	
	for (uint bounce = 0; bounce < SPP; ++bounce)
	{
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
		finalColor += payload.color.rgb;
	}
	finalColor /= SPP;
	
	float3 bloom = max(0.0f.xxx, finalColor - 0.75f.xxx) * 0.7f;
	finalColor += bloom;
	
	finalColor = pow(finalColor, (1.0f / 1.8f).xxx);
  
	float luminance = dot(finalColor, float3(0.299f, 0.587f, 0.114f));
	finalColor = lerp(luminance.xxx, finalColor, 1.4f);
	finalColor += float3(0.05f, 0.03f, 0.01f);
	finalColor = clamp(finalColor, 0.0f, 1.0f);
	
	g_output[pixelCoord] = float4(finalColor, 1.0f);
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

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
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
	float3 geometricNormal = normal;
	float4 tangentPacked = v0.tangent * bary.x + v1.tangent * bary.y + v2.tangent * bary.z;
	float3 tangentObj = tangentPacked.xyz;
	float tangentSign = tangentPacked.w;
	float3 tangent = normalize(mul(tangentObj, (float3x3) inst.worldMatrix));
	tangent = normalize(tangent - normal * dot(tangent, normal));
	float3 bitangent = normalize(cross(normal, tangent) * tangentSign);
	
	float2 uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;
	TerrainSample terrainSample;
	bool useTerrainSplat = 0 != (mat.materialFlags & MATERIAL_FLAG_TERRAIN_SPLAT);
	if (useTerrainSplat)
	{
		terrainSample = SampleTerrainSplat(mat, uv);
		normal = normalize(
			tangent * terrainSample.normalTS.x +
			bitangent * terrainSample.normalTS.y +
			normal * terrainSample.normalTS.z
		);
	}
	else if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_NORMAL_MAP))
	{
		Texture2D normalTexture = ResourceDescriptorHeap[mat.normalTextureIdx];
		float2 encodedXY = normalTexture.SampleLevel(g_sampler, uv, 0).xy * 2.0f - 1.0f;
		float3 normalSample;
		normalSample.xy = encodedXY;
		normalSample.z = sqrt(saturate(1.0f - dot(encodedXY, encodedXY)));
		normal = normalize(
			tangent * normalSample.x +
			bitangent * normalSample.y +
			normal * normalSample.z
		);
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
	float NdotVGeom = max(dot(geometricNormal, V), 0.0f);
	float NdotVShading = max(dot(normal, V), 0.0f);
	float3 F0 = lerp(0.04f.xxx, albedo, metallic);
	
	if (payload.recursionDepth >= MAX_RECURSION_DEPTH)
	{
		return;
	}
	
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
	
	float3 indirectF = FresnelSchlick(NdotVGeom, F0);
	float3 kS_indirect = indirectF;
	float3 kD_indirect = (1.0 - kS_indirect) * (1.0 - metallic);
	
	float diffuseContrib = dot(kD_indirect * albedo, float3(0.299f, 0.587f, 0.114f));
	float specContrib = dot(kS_indirect, float3(0.299f, 0.587f, 0.114f));
	
	float totalContrib = diffuseContrib + specContrib;
	float specProb = (totalContrib > 0.0001f) ? (specContrib / totalContrib) : 0.0f;
	specProb = clamp(specProb, 0.01f, 0.99f);
	
	float xi = RandomValue(rngSeed);
	bool chooseSpec = (xi < specProb);
	
	float3 L;
	float3 f; // BSDF 값
	float pdf; // 샘플링 pdf (mixture)
	float3 weight; // 1-step throughput
	
	if (!chooseSpec)
	{
		L = CosineHemisphere(geometricNormal, rngSeed);
		float NdotL = max(dot(geometricNormal, L), 0.0f);
		if (NdotL <= 0.0f)
		{
			return;
		}

		float3 f_d = kD_indirect * albedo * ao / PI;
		float pdf_lobe = NdotL / PI;
		float branchProb = 1.0f - specProb;

		pdf = max(branchProb * pdf_lobe, EPSILON);
		f = f_d;
		weight = f * NdotL / pdf;
	}
	else
	{
		L = SampleGGX(normal, V, roughness, rngSeed);
		float NdotL = max(dot(normal, L), 0.0f);
		if (NdotL <= 0.0f)
		{
			return;
		}
		
		float3 H = normalize(V + L);

		float NDF = DistributionGGX(normal, H, roughness);
		float G = GeometrySmith(normal, V, L, roughness);
		float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

		float3 numerator = NDF * G * F;
		float denominator = 4.0f * NdotVShading * NdotL;
		float3 f_s = numerator / max(denominator, EPSILON);
		
		float pdf_lobe = GGX_PDF(normal, H, V, L, roughness);
		float branchProb = specProb;

		pdf = max(branchProb * pdf_lobe, EPSILON);
		f = f_s;
		weight = f * NdotL / pdf;
	}
	
	float maxW = max(weight.x, max(weight.y, weight.z));
	if (maxW < 1e-5f)
	{
		payload.color = kD_indirect * albedo * ao;
		if (0 != (mat.materialFlags & MATERIAL_FLAG_EMISSIVE_MAP))
		{
			Texture2D emissiveTexture = ResourceDescriptorHeap[mat.emissiveTextureIdx];
			payload.color += emissiveTexture.SampleLevel(g_sampler, uv, 0).rgb;
		}
		return;
	}

	RayDesc nextRay;
	nextRay.Origin = hitPos + geometricNormal * 0.01f;
	nextRay.Direction = normalize(L);
	nextRay.TMin = RAY_TMIN;
	nextRay.TMax = RAY_TMAX;

	RayPayload child;
	child.color = 1.0f.xxx;
	child.recursionDepth = payload.recursionDepth + 1;

	TraceRay(
		g_scene,
		RAY_FLAG_FORCE_OPAQUE,
		0xFF,
		0,
		0,
		0,
		nextRay,
		child
	);

	payload.color = weight * child.color;
	if (0 != (mat.materialFlags & MATERIAL_FLAG_EMISSIVE_MAP))
	{
		Texture2D emissiveTexture = ResourceDescriptorHeap[mat.emissiveTextureIdx];
		payload.color += emissiveTexture.SampleLevel(g_sampler, uv, 0).rgb;
	}
}
