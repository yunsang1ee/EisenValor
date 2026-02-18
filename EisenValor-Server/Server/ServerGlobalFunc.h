#pragma once
static inline FB_STRUCTS::Vec3 Vec3ToFlatVec3(const Vec3& v)
{
	return FB_STRUCTS::Vec3{ v.x, v.y, v.z };
}

static inline Vec3 FlatVec3ToVec3(const FB_STRUCTS::Vec3& v)
{
	return Vec3{ v.x(), v.y(), v.z() };
}

static inline Vec3 FlatVec3ToVec3(const FB_STRUCTS::Vec3* v)
{
	return Vec3{ v->x(), v->y(), v->z() };
}

static inline float Deg2Rad(const float degree)
{
	return degree * (DirectX::XM_PI / 180.f);
}

template<typename Enum> requires std::is_enum_v<Enum>
static inline constexpr uint8 etou8(const Enum e)
{
	return static_cast<uint8>(e);
}

inline constexpr GENERAL_SUB_STATE_TYPE operator|(const GENERAL_SUB_STATE_TYPE a, const GENERAL_SUB_STATE_TYPE b)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator|=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b)
{
	a = a | b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator&(const GENERAL_SUB_STATE_TYPE a, const GENERAL_SUB_STATE_TYPE b)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator&=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b)
{
	a = a & b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator^(const GENERAL_SUB_STATE_TYPE a, const GENERAL_SUB_STATE_TYPE b)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator^=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b)
{
	a = a ^ b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator~(const GENERAL_SUB_STATE_TYPE a)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(~static_cast<uint8>(a));
}