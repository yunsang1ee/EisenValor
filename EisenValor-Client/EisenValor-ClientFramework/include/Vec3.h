#pragma once
// -----------------------------------------------
// Aligned Vec3
// -----------------------------------------------
using Vec3 = DX::XMFLOAT3;

struct alignas(16) Vec3v
{
	using dimensionTag = std::integral_constant<int, 3>;
	DirectX::XMVECTOR v;

	__forceinline Vec3v() noexcept : v(DirectX::XMVectorZero()) {}
	__forceinline Vec3v(float x, float y, float z) noexcept : v(DirectX::XMVectorSet(x, y, z, 0.0f)) {}
	__forceinline explicit Vec3v(DirectX::FXMVECTOR vv) noexcept : v(vv) {}
	__forceinline explicit Vec3v(const DirectX::XMFLOAT3A& f) noexcept : v(DirectX::XMLoadFloat3A(&f)) {}
	__forceinline explicit Vec3v(const DirectX::XMFLOAT3& f) noexcept : v(DirectX::XMLoadFloat3(&f)) {}

	__forceinline DirectX::XMVECTOR Vector() const noexcept { return v; }

	__forceinline DirectX::XMFLOAT3A ToFloat3A() const noexcept
	{
		DirectX::XMFLOAT3A out;
		DirectX::XMStoreFloat3A(&out, v);
		return out;
	}
	__forceinline DirectX::XMFLOAT3 ToFloat3() const noexcept
	{
		DirectX::XMFLOAT3 out;
		DirectX::XMStoreFloat3(&out, v);
		return out;
	}
	__forceinline explicit operator DirectX::XMFLOAT3A() const noexcept { return ToFloat3A(); } // 명시적

	// 연산자들
	__forceinline Vec3v operator+(const Vec3v& r) const noexcept { return Vec3v(DirectX::XMVectorAdd(v, r.v)); }
	__forceinline Vec3v operator-(const Vec3v& r) const noexcept { return Vec3v(DirectX::XMVectorSubtract(v, r.v)); }
	__forceinline Vec3v operator*(const Vec3v& r) const noexcept { return Vec3v(DirectX::XMVectorMultiply(v, r.v)); }
	__forceinline Vec3v operator/(const Vec3v& r) const noexcept { return Vec3v(DirectX::XMVectorDivide(v, r.v)); }
	__forceinline Vec3v operator*(float s) const noexcept { return Vec3v(DirectX::XMVectorScale(v, s)); }
	__forceinline Vec3v operator/(float s) const noexcept { return Vec3v(DirectX::XMVectorScale(v, 1.0f / s)); }
	friend __forceinline Vec3v operator*(float s, const Vec3v& a) noexcept
	{
		return Vec3v(DirectX::XMVectorScale(a.v, s));
	}

	__forceinline Vec3v& operator+=(const Vec3v& r) noexcept
	{
		v = DirectX::XMVectorAdd(v, r.v);
		return *this;
	}
	__forceinline Vec3v& operator-=(const Vec3v& r) noexcept
	{
		v = DirectX::XMVectorSubtract(v, r.v);
		return *this;
	}
	__forceinline Vec3v& operator*=(float s) noexcept
	{
		v = DirectX::XMVectorScale(v, s);
		return *this;
	}
	__forceinline Vec3v& operator/=(float s) noexcept
	{
		v = DirectX::XMVectorScale(v, 1.0f / s);
		return *this;
	}

	__forceinline Vec3v operator-() const noexcept { return Vec3v(DirectX::XMVectorNegate(v)); }

	__forceinline float Length() const noexcept { return DirectX::XMVectorGetX(DirectX::XMVector3Length(v)); }
	__forceinline float LengthSq() const noexcept { return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(v)); }
	__forceinline Vec3v Normalize() const noexcept { return Vec3v(DirectX::XMVector3Normalize(v)); }
	__forceinline float Dot(const Vec3v& r) const noexcept
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(v, r.v));
	}
	__forceinline Vec3v Cross(const Vec3v& r) const noexcept { return Vec3v(DirectX::XMVector3Cross(v, r.v)); }

	__forceinline bool NearEqual(const Vec3v& r, float eps = 1e-6f) const noexcept
	{
		return DirectX::XMVector3NearEqual(v, r.v, DirectX::XMVectorReplicate(eps));
	}

	static __forceinline Vec3v Zero() noexcept { return Vec3v(DirectX::XMVectorZero()); }
	static __forceinline Vec3v One() noexcept { return Vec3v(DirectX::XMVectorSplatOne()); }
	static __forceinline Vec3v Right() noexcept { return Vec3v(DirectX::g_XMIdentityR0); }	 // (1,0,0,0)
	static __forceinline Vec3v Up() noexcept { return Vec3v(DirectX::g_XMIdentityR1); }		 // (0,1,0,0)
	static __forceinline Vec3v Forward() noexcept { return Vec3v(DirectX::g_XMIdentityR2); } // (0,0,1,0)
};
static_assert(alignof(Vec3v) == 16, "Vec3v must be 16-byte aligned");
