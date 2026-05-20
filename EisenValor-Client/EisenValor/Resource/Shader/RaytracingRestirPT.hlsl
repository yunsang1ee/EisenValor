#define HLSL
#include "RaytracingCommon.h"
#include "RaytracingMaterialEmission.hlsli"
#include "RaytracingTerrain.hlsli"
#include "RaytracingEnvironment.hlsli"
#include "RaytracingPostProcess.hlsli"
#include "RaytracingNormal.hlsli"
#include "RestirReservoir.hlsli"

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float4> g_output : register(u0, space0);
RWStructuredBuffer<RestirPrimaryHit> g_restirPrimaryHitCurrent : register(u2, space0);
RWStructuredBuffer<RestirReservoir> g_restirReservoirInitial : register(u3, space0);

cbuffer CameraConstants : register(b0, space0)
{
    float4x4 g_viewProjInverse;
};

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

cbuffer RestirCandidateConstants : register(b3, space0)
{
    uint g_restirCandidateEnabled;
    uint g_restirDebugView;
    uint g_restirScreenWidth;
    uint g_restirScreenHeight;
};

SamplerState g_sampler : register(s0, space0);

static const uint MAX_RECURSION_DEPTH = 6;
static const uint SPP = 4;
static const uint INVALID_TEXTURE_INDEX = 0xffffffffu;
static const float PT_OUTPUT_EXPOSURE = 0.75f;
static const float PT_BLOOM_THRESHOLD = 1.0f;
static const float PT_BLOOM_INTENSITY = 0.75f;
static const float PT_VISIBLE_NORMAL_STRENGTH = 2.5f;

// 0: off, 1: shading normal, 2: geometric normal, 3: NdotV(shading/geom), 4: albedo, 5: metallic/roughness/ao.
static const uint PT_DEBUG_VIEW = 0;

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

float3 StrengthenTangentSpaceNormal(float3 normalTS, float strength)
{
    return normalize(float3(normalTS.xy * strength, max(normalTS.z, 0.0f)));
}

bool UsePhysicalRenderingMode()
{
    return EMISSION_VIEW_PHYSICAL == g_emissionViewMode;
}

#define RAYTRACING_ENABLE_GGX_SAMPLING 1
#include "RaytracingLighting.hlsli"

[shader("raygeneration")]
void RayGenMain()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;
    uint pixelIndex = pixelCoord.y * screenSize.x + pixelCoord.x;
	
    uint rngSeed = pixelIndex * 9781u
					 ^ pixelCoord.y * 6271u
					 ^ screenSize.x * 7919u
                     ^ (g_frameSeed + 1u) * 104729u
                     ^ 124623u;
	
    float3 finalColor = 0.0f.xxx;
    RestirReservoir restirReservoir = RestirMakeEmptyReservoir();
	
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
        ray.TMax = RAY_TMAX;

        RayPayload payload = MakeDefaultRayPayload(0);
		
        TraceRay(g_scene,
				 RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
				 0xFF,
				 0, 0, 0,
				 ray,
				 payload);
        finalColor += payload.color.rgb;

        if (0u != g_restirCandidateEnabled && 0u != (payload.primaryHitFlags & RESTIR_PRIMARY_HIT_VALID))
        {
            RestirPathSample candidate = RestirMakeCandidateFromPayload(payload);
            RestirUpdateReservoir(restirReservoir, candidate, candidate.contributionTarget.w, RandomValue(rngSeed));
        }
    }
    finalColor /= SPP;

    if (0u != g_restirCandidateEnabled)
    {
        RestirPrimaryHit primaryHit = RestirPrimaryHitFromSample(restirReservoir.sample, restirReservoir.flags);
        g_restirReservoirInitial[pixelIndex] = restirReservoir;
        g_restirPrimaryHitCurrent[pixelIndex] = primaryHit;

        if (0u != g_restirDebugView)
        {
            g_output[pixelCoord] = float4(RestirDebugColor(restirReservoir, primaryHit, g_restirDebugView - 1u), 1.0f);
            return;
        }
    }

    float3 outputColor = finalColor;
    if (PT_DEBUG_VIEW == 0 && !UsePhysicalRenderingMode())
    {
        outputColor *= PT_OUTPUT_EXPOSURE;
        float3 bloom = max(0.0f.xxx, outputColor - PT_BLOOM_THRESHOLD.xxx) * PT_BLOOM_INTENSITY;
        outputColor += bloom;
    }

    g_output[pixelCoord] = float4(max(0.0f.xxx, outputColor), 1.0f);
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    payload.color = SampleEnvironment(WorldRayDirection(), g_environmentMode);
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
	
    float3 normalObj = SafeNormalizeRay(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z, float3(0.0f, 1.0f, 0.0f));
    float3 normal = SafeNormalizeRay(mul(normalObj, (float3x3) inst.worldInverse), float3(0.0f, 1.0f, 0.0f));
    float3 geometricNormal = normal;
    float3 visibleNormal = normal;
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
    float visibleNormalStrength = UsePhysicalRenderingMode() ? 1.0f : PT_VISIBLE_NORMAL_STRENGTH;
    if (useTerrainSplat)
    {
        terrainSample = SampleTerrainSplat(mat, uv, g_terrainSurfaces, g_sampler);
        float3 visibleNormalTS = StrengthenTangentSpaceNormal(terrainSample.normalTS, visibleNormalStrength);
        normal = TangentNormalToWorld(terrainSample.normalTS, tangent, bitangent, normal);
        visibleNormal = normalize(
            tangent * visibleNormalTS.x +
            bitangent * visibleNormalTS.y +
            geometricNormal * visibleNormalTS.z
        );
    }
    else if (0 != (mat.materialFlags & MATERIAL_FLAG_USE_NORMAL_MAP))
    {
        Texture2D normalTexture = ResourceDescriptorHeap[mat.normalTextureIdx];
        float2 encodedXY = normalTexture.SampleLevel(g_sampler, uv, 0).xy * 2.0f - 1.0f;
        float3 normalSample;
        normalSample.xy = encodedXY;
        normalSample.z = sqrt(saturate(1.0f - dot(encodedXY, encodedXY)));
        float3 visibleNormalSample = StrengthenTangentSpaceNormal(normalSample, visibleNormalStrength);
        normal = TangentNormalToWorld(normalSample, tangent, bitangent, normal);
        visibleNormal = normalize(
            tangent * visibleNormalSample.x +
            bitangent * visibleNormalSample.y +
            geometricNormal * visibleNormalSample.z
        );
    }
    
    if (dot(normal, geometricNormal) < 0.0f)
    {
        normal = geometricNormal;
    }
    if (dot(visibleNormal, geometricNormal) < 0.0f)
    {
        visibleNormal = geometricNormal;
    }
    if (dot(normal, WorldRayDirection()) > 0.0f)
    {
        normal = -normal;
    }
    if (dot(visibleNormal, WorldRayDirection()) > 0.0f)
    {
        visibleNormal = -visibleNormal;
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

    if (payload.recursionDepth == 0)
    {
        payload.primaryHitPosition = hitPos;
        payload.primaryHitDistance = RayTCurrent();
        payload.primaryHitNormal = geometricNormal;
        payload.primaryHitFlags = RESTIR_PRIMARY_HIT_VALID;
        payload.instanceId = inst.instanceID;
        payload.materialId = geo.materialIdx;
        payload.geometryId = inst.geoInfoBaseIdx + GeometryIndex();
        payload.roughness = roughness;
    }

    float3 V = -normalize(WorldRayDirection());
    float NdotVGeom = max(dot(geometricNormal, V), 0.0f);
    float NdotVShading = max(dot(visibleNormal, V), 0.0f);
    float3 F0 = lerp(0.04f.xxx, albedo, metallic);
    float3 emissive = EvaluateMaterialEmission(mat, uv, g_sampler);
    if (payload.recursionDepth == 0 && PT_DEBUG_VIEW != 0)
    {
        if (PT_DEBUG_VIEW == 1)
        {
            payload.color = visibleNormal * 0.5f + 0.5f;
        }
        else if (PT_DEBUG_VIEW == 2)
        {
            payload.color = geometricNormal * 0.5f + 0.5f;
        }
        else if (PT_DEBUG_VIEW == 3)
        {
            payload.color = float3(NdotVShading, NdotVGeom, 0.0f);
        }
        else if (PT_DEBUG_VIEW == 4)
        {
            payload.color = albedo;
        }
        else
        {
            payload.color = float3(metallic, roughness, ao);
        }
        return;
    }

    if (max(max(emissive.x, emissive.y), emissive.z) > 0.0f)
    {
        payload.color = (0 == payload.recursionDepth)
            ? EvaluateCameraVisibleMaterialEmission(mat, uv, g_emissionViewMode, g_sampler)
            : emissive;
        return;
    }
	
    if (payload.recursionDepth >= MAX_RECURSION_DEPTH)
    {
        payload.color = emissive;
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
	
    float3 indirectF = FresnelSchlick(NdotVShading, F0);
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
        L = CosineHemisphere(visibleNormal, rngSeed);
        float NdotL = max(dot(visibleNormal, L), 0.0f);
        if (NdotL <= 0.0f)
        {
            payload.color = emissive;
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
        if (roughness < 0.15f)
        {
            L = reflect(WorldRayDirection(), visibleNormal);
            float NdotL = max(dot(visibleNormal, L), 0.0f);
            if (NdotL <= 0.0f)
            {
                payload.color = emissive;
                return;
            }

            float3 F = FresnelSchlick(NdotVShading, F0);
            pdf = max(specProb, EPSILON);
            weight = F / pdf;
        }
        else
        {
            L = SampleGGX(visibleNormal, V, roughness, rngSeed);
            float NdotL = max(dot(visibleNormal, L), 0.0f);
            if (NdotL <= 0.0f)
            {
                payload.color = emissive;
                return;
            }
		
            float3 H = normalize(V + L);

            float NDF = DistributionGGX(visibleNormal, H, roughness);
            float G = GeometrySmith(visibleNormal, V, L, roughness);
            float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

            float3 numerator = NDF * G * F;
            float denominator = 4.0f * NdotVShading * NdotL;
            float3 f_s = numerator / max(denominator, EPSILON);
		
            float pdf_lobe = GGX_PDF(visibleNormal, H, V, L, roughness);
            float branchProb = specProb;

            pdf = max(branchProb * pdf_lobe, EPSILON);
            f = f_s;
            weight = f * NdotL / pdf;
        }
    }
	
    float maxW = max(weight.x, max(weight.y, weight.z));
    if (maxW < 1e-5f)
    {
        payload.color = emissive;
        return;
    }

    RayDesc nextRay;
    nextRay.Origin = hitPos + geometricNormal * 0.01f;
    nextRay.Direction = normalize(L);
    nextRay.TMin = RAY_TMIN;
    nextRay.TMax = RAY_TMAX;

    RayPayload child = MakeDefaultRayPayload(payload.recursionDepth + 1);

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

    payload.color = emissive + weight * child.color;
}
