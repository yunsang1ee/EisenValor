#ifndef RAYTRACING_PAYLOAD_HLSLI
#define RAYTRACING_PAYLOAD_HLSLI

struct StandardRayPayload
{
    float3 color;
    uint recursionDepth;
};

struct RestirRayPayload
{
    float3 color;
    uint recursionDepth;

    float3 targetContribution;
    float sourcePdf;

    float lightPdf;
    uint pathFlags;
    uint pixelIndex;
    uint primaryHitFlags;

    uint reconnectInstanceId;
    uint reconnectInstanceGeneration;
    uint reconnectGeometryIndex;
    uint reconnectPrimitiveIndex;
    uint reconnectBarycentrics;
};

void InitializeRayPayloadData(inout StandardRayPayload payload)
{
}

void InitializeRayPayloadData(inout RestirRayPayload payload)
{
    payload.targetContribution = 0.0f.xxx;
    payload.sourcePdf = 1.0f;
    payload.lightPdf = 0.0f;
    payload.pathFlags = 0u;
    payload.pixelIndex = 0xffffffffu;
    payload.primaryHitFlags = 0u;
    payload.reconnectInstanceId = 0xffffffffu;
    payload.reconnectInstanceGeneration = 0u;
    payload.reconnectGeometryIndex = 0xffffffffu;
    payload.reconnectPrimitiveIndex = 0xffffffffu;
    payload.reconnectBarycentrics = 0u;
}

template<typename TPayload>
TPayload MakeDefaultRayPayload(uint recursionDepth)
{
    TPayload payload;
    payload.color = 0.0f.xxx;
    payload.recursionDepth = recursionDepth;
    InitializeRayPayloadData(payload);
    return payload;
}

#endif
