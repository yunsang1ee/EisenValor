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
