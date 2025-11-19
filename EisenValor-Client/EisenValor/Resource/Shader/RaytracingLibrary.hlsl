#define HLSL
#include "RaytracingCommon.hlsli"

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
    uint recursionDepth;
};

static const float PI = 3.14159265359f;
static const float EPSILON = 0.0000001f;
static const float RAY_TMIN = 0.001f;
static const float RAY_TMAX = 10000.0f;
static const uint MAX_RECURSION_DEPTH = 7;

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

// 직교 좌표계 생성
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

[shader("raygeneration")]
void RayGenMain()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;
    float2 uv = (float2(pixelCoord) + 0.5f) / float2(screenSize);
    float2 ndc = uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    
    float4 target = mul(float4(ndc, 1.0f, 1.0f), g_viewProjInverse);
    target /= target.w;
    
    float4 cameraPos = mul(float4(0, 0, 0, 1), g_viewProjInverse);
    cameraPos /= cameraPos.w;
		
    RayDesc ray;
    ray.Origin = cameraPos.xyz;
    ray.Direction = normalize(target.xyz - cameraPos.xyz);
    ray.TMin = RAY_TMIN;
    ray.TMax = RAY_TMAX;
	
    RayPayload payload;
    payload.color = float3(0, 0, 0);
    payload.recursionDepth = 0; // 초기 깊이 0으로 설정
	
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
    g_output[pixelCoord] = float4(finalColor, 1.0f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint instanceID = InstanceID();
    Material material = g_materials[instanceID];
    uint geoBase = g_instGeoBase[instanceID];

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
	
	// 균등 스케일 가정
    float3 normalObj = normalize(
        v0.normal * bary.x + 
        v1.normal * bary.y + 
        v2.normal * bary.z
    );
    float3x3 objectToWorld = (float3x3) ObjectToWorld3x4();
    float3 normal = normalize(mul(normalObj, transpose(objectToWorld)));
    if (dot(normal, WorldRayDirection()) > 0.0f)
        normal = -normal;
        
    float3 V = -normalize(WorldRayDirection());
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float3 lightColor = float3(1.0, 0.95, 0.8) * 3.0;
    float3 H = normalize(V + lightDir);
    
    float3 albedo = material.albedo;
    float metallic = material.metallic;    
    float roughness = max(material.roughness, 0.04);    
    float3 F0 = lerp(0.04f.xxx, albedo, metallic);
    
    float NDF = DistributionGGX(normal, H, roughness);    
    float G = GeometrySmith(normal, V, lightDir, roughness);
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    
    float3 numerator = NDF * G * F;    
    float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, lightDir), 0.0);    
    float3 specular = numerator / max(denominator, EPSILON);
    
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);    
    float NdotL = saturate(dot(normal, lightDir));
    
    float3 Lo = (kD * albedo / PI + specular) * lightColor * NdotL;
	
    float3 shadowOrigin = hitPos + normal * 0.1f;
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
    if (metallic > 0.1f && payload.recursionDepth < MAX_RECURSION_DEPTH)
    {
        float3 reflectDir = reflect(WorldRayDirection(), normal);
		
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
			RAY_FLAG_FORCE_OPAQUE,
			0xFF,
			0,
			0,
			0,
			reflectRay,
			reflectPayload
		);
		
        reflectedColor = reflectPayload.color * metallic * (1.0f - roughness);
    }
	
    float ao = bary.x * 0.3f + 0.7f;
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * (1.0f + metallic * 0.5f);
    float3 emissive = material.emissive * material.emissiveStrength;
	
    payload.color = ambient * ao + Lo + reflectedColor + emissive;
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
	//payload.color = 0.0f.xxx;
    float3 rayDir = WorldRayDirection();
    float t = 0.5f * (rayDir.y + 1.0f);
    payload.color = lerp(1.0f.xxx, float3(0.5f, 0.7f, 1.0f), t);
}
