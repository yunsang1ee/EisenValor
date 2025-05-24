#pragma once
#include "IntX.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Color.h"
#include "Quaternion.h"
#include "Plane.h"
#include "Mat4.h"




namespace DirectX
{
	[[nodiscard]]
	static constexpr Vec3 Min(const Vec3& a, const Vec3& b) noexcept {
		return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
	}
	[[nodiscard]]
	static constexpr Vec3 Max(const Vec3& a, const Vec3& b) noexcept {
		return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
	}
	[[nodiscard]]
	static constexpr Vec3 Clamp(const Vec3& v, const Vec3& vmin, const Vec3& vmax) noexcept {
		return Max(vmin, Min(v, vmax));
	}
	[[nodiscard]]
	static constexpr Vec3 Lerp(const Vec3& a, const Vec3& b, float t) noexcept {
		return Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
	}

	inline Int2 XMConvertToInt2(const XMFLOAT2& f) { return Int2(int32_t(f.x), int32_t(f.y)); }
	inline XMFLOAT2 XMConvertToXMFLOAT2(const Int2& i) { return XMFLOAT2(float(i.x), float(i.y)); }
}