#pragma once
static inline FB_STRUCTS::Vec3 Vec3ToFlatVec3(const Vec3& v)
{
	return FB_STRUCTS::Vec3{ v.x, v.y, v.z };
}

static inline Vec3 FlatVec3ToVec3(const FB_STRUCTS::Vec3& v)
{
	return Vec3{ v.x(), v.y(), v.z() };
}

template<typename Enum>
static inline constexpr uint8 etou8(const Enum e)
{
	return static_cast<uint8>(e);
}