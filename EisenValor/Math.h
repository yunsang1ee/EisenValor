#pragma once
#include "IntX.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Color.h"
#include "Quaternion.h"
#include "Plane.h"
#include "Mat4.h"


namespace Math
{
	template<typename T>
	concept VecNA = requires(T v)
	{
		typename T::dimension_tag;
		requires std::is_base_of_v<std::integral_constant<int, T::dimension_tag::value>, typename T::dimension_tag>;
		{ v.Vector() } -> std::same_as<DirectX::XMVECTOR>;
	};
}

namespace Math::detail
{
#pragma region NearEqual
	template<VecNA Vec, typename Tag>
	struct NearEqualDispatcher;

	template<VecNA Vec>
	struct NearEqualDispatcher<Vec, std::integral_constant<int, 2>>
	{
		static bool call(const Vec& a, const Vec& b, float epsilon)
		{
			return DirectX::XMVector2NearEqual(
				a.Vector(), b.Vector(),
				DirectX::XMVectorReplicate(epsilon)
			);
		}
	};

	template<VecNA Vec>
	struct NearEqualDispatcher<Vec, std::integral_constant<int, 3>>
	{
		static bool call(const Vec& a, const Vec& b, float epsilon)
		{
			return DirectX::XMVector3NearEqual(
				a.Vector(), b.Vector(),
				DirectX::XMVectorReplicate(epsilon)
			);
		}
	};

	template<VecNA Vec>
	struct NearEqualDispatcher<Vec, std::integral_constant<int, 4>>
	{
		static bool call(const Vec& a, const Vec& b, float epsilon)
		{
			return DirectX::XMVector4NearEqual(
				a.Vector(), b.Vector(),
				DirectX::XMVectorReplicate(epsilon)
			);
		}
	};
#pragma endregion

}

namespace Math
{
	template<VecNA Vec>
	[[nodiscard]]
	inline bool NearEqual(const Vec& a, const Vec& b, float epsilon = 1e-6f)
	{
		return detail::NearEqualDispatcher<Vec, Vec::dimension_tag>
			::call(a, b, epsilon);
	}

	/*
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
	*/
}