#ifndef RAYTRACING_ENVIRONMENT_HLSLI
#define RAYTRACING_ENVIRONMENT_HLSLI

float3 SampleNightEnvironment(float3 rayDir)
{
    float skyT = smoothstep(-0.05f, 0.65f, rayDir.y);
    float groundToSkyT = smoothstep(-0.02f, 0.02f, rayDir.y);

    float3 nightZenith = float3(5.0f, 18.0f, 62.0f) / 255.0f;
    float3 nightHorizon = float3(16.0f, 8.0f, 38.0f) / 255.0f;
    float3 groundColor = float3(2.0f, 3.0f, 10.0f) / 255.0f;
    float3 nightSky = lerp(nightHorizon, nightZenith, skyT);

    return lerp(groundColor, nightSky, groundToSkyT);
}

float3 GetEnvironmentSunDirection()
{
    return normalize(float3(0.35f, 0.75f, 0.45f));
}

float3 GetEnvironmentSunRadiance(uint environmentMode)
{
    return (0u != environmentMode)
        ? float3(1.0f, 0.95f, 0.8f) * 2.5f
        : float3(5.0f, 18.0f, 62.0f) / 255.0f * 0.25f;
}

float GetEnvironmentSunAngularRadius(uint environmentMode)
{
    return (0u != environmentMode) ? 0.04f : 0.08f;
}

float3 AtmosphereGradient(float3 rayDir, float3 sunDir)
{
    float height = rayDir.y;
    float sunHeight = sunDir.y;

    float horizonFade = smoothstep(-0.02f, 0.05f, height);

    float3 zenithColor = lerp(float3(0.3f, 0.4f, 0.6f), float3(0.5f, 0.7f, 1.0f), smoothstep(-0.2f, 0.5f, sunHeight));
    float3 horizonColor = lerp(float3(0.8f, 0.4f, 0.2f), float3(0.9f, 0.8f, 0.7f), smoothstep(-0.2f, 0.2f, sunHeight));

    float3 skyColor = lerp(horizonColor, zenithColor, smoothstep(0.0f, 0.4f, height));

    float sunInfluence = max(0.0f, dot(rayDir, sunDir));
    float3 sunTint = float3(1.0f, 0.8f, 0.6f) * pow(sunInfluence, 8.0f) * 0.3f;

    return skyColor * horizonFade + sunTint;
}

float3 RayleighScattering(float3 rayDir, float3 sunDir)
{
    float cosTheta = dot(rayDir, sunDir);
    float rayleighPhase = 3.0f / (16.0f * PI) * (1.0f + cosTheta * cosTheta);

    float3 wavelengths = float3(0.65f, 0.57f, 0.475f);
    float3 scatterCoeff = 1.0f / pow(wavelengths, 4.0f.xxx);

    return scatterCoeff * rayleighPhase * 0.3f;
}

float3 MieScattering(float3 rayDir, float3 sunDir)
{
    float cosTheta = dot(rayDir, sunDir);
    float g = 0.76f;
    float g2 = g * g;

    float miePhase = (3.0f * (1.0f - g2)) / (2.0f * (2.0f + g2)) *
        (1.0f + cosTheta * cosTheta) / pow(1.0f + g2 - 2.0f * g * cosTheta, 1.5f);

    return 1.0f.xxx * miePhase * 0.1f;
}

float3 SunHalo(float3 rayDir, float3 sunDir)
{
    float sunDot = max(0.0f, dot(rayDir, sunDir));

    float sunCore = pow(sunDot, 2000.0f) * 2.0f;
    float halo1 = pow(sunDot, 200.0f) * 0.8f;
    float halo2 = pow(sunDot, 50.0f) * 0.4f;
    float halo3 = pow(sunDot, 20.0f) * 0.2f;
    float corona = pow(sunDot, 10.0f) * 0.1f;

    float3 sunColor = float3(1.0f, 0.95f, 0.8f);
    float3 haloColor1 = float3(1.0f, 0.9f, 0.7f);
    float3 haloColor2 = float3(1.0f, 0.8f, 0.6f);
    float3 coronaColor = float3(0.9f, 0.7f, 0.5f);

    return sunCore * sunColor +
        halo1 * haloColor1 +
        halo2 * haloColor2 +
        halo3 * haloColor1 * 0.5f +
        corona * coronaColor;
}

float3 SampleDaySkyEnvironment(float3 rayDir)
{
    float3 sunDir = GetEnvironmentSunDirection();
    float3 sky = AtmosphereGradient(rayDir, sunDir);
    float3 rayleigh = RayleighScattering(rayDir, sunDir) * 0.08f;
    float3 mie = MieScattering(rayDir, sunDir) * 0.15f;
    float groundToSkyT = smoothstep(-0.02f, 0.02f, rayDir.y);
    float3 groundColor = float3(0.18f, 0.20f, 0.18f);
    return lerp(groundColor, sky + rayleigh + mie, groundToSkyT);
}

float CloudNoise(float3 rayDir)
{
    float3 p = rayDir * 5.0f;
    return frac(sin(dot(p, float3(12.9898f, 78.233f, 45.164f))) * 43758.5453f) * 0.5f + 0.5f;
}

float3 SampleDayEnvironment(float3 rayDir)
{
    float3 sunDir = GetEnvironmentSunDirection();
    float3 sky = SampleDaySkyEnvironment(rayDir);
    float3 sun = SunHalo(rayDir, sunDir) * 0.35f;
    return sky + sun;
}

float3 SampleEnvironment(float3 rayDir, uint environmentMode)
{
    return (0u != environmentMode) ? SampleDayEnvironment(rayDir) : SampleNightEnvironment(rayDir);
}

float3 SampleEnvironmentWithoutSun(float3 rayDir, uint environmentMode)
{
    return (0u != environmentMode) ? SampleDaySkyEnvironment(rayDir) : SampleNightEnvironment(rayDir);
}

#endif // RAYTRACING_ENVIRONMENT_HLSLI
