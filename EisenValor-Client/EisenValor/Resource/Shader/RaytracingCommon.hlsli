// =======================================================================================
// Raytracing Common Definitions
// =======================================================================================

#ifndef RAYTRACING_COMMON_HLSLI
#define RAYTRACING_COMMON_HLSLI

// Material masks
#define MATERIAL_HAS_ALBEDO_TEXTURE    (1 << 0)
#define MATERIAL_HAS_NORMAL_TEXTURE    (1 << 1) 
#define MATERIAL_HAS_METALLIC_TEXTURE  (1 << 2)
#define MATERIAL_HAS_ROUGHNESS_TEXTURE (1 << 3)
#define MATERIAL_HAS_EMISSIVE_TEXTURE  (1 << 4)
#define MATERIAL_IS_TRANSPARENT        (1 << 5)

// Instance types
#define INSTANCE_TYPE_GROUND    0
#define INSTANCE_TYPE_PLAYER    1
#define INSTANCE_TYPE_NPC       2

// Ray types
#define RAY_TYPE_PRIMARY        0
#define RAY_TYPE_REFLECTION     1

#endif // RAYTRACING_COMMON_HLSLI