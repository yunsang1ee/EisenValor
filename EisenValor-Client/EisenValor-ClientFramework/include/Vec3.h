#pragma once
// -----------------------------------------------
// Aligned Vec3
// -----------------------------------------------
using Vec3 = DX::XMFLOAT3;

struct alignas(16) Vec3A
{
	using dimensionTag = std::integral_constant<int, 3>;

	DirectX::XMVECTOR v;
	/*
	union
	{
		struct alignas(16)
		{
			float x;
			float y;
			float z;
			[[maybe_unused]] float _padding;
		};
		DirectX::XMVECTOR v;
	};
	*/

#pragma region Constructor & Translator
	Vec3A() noexcept 
		: v{ DirectX::XMVectorZero() } {}
	Vec3A(float x_, float y_, float z_) noexcept 
		: v{ DirectX::XMVectorSet(x_, y_, z_, 0.0f) } {}

	[[nodiscard]]
	explicit Vec3A(const DirectX::XMFLOAT3A& v_) noexcept 
		: v(DirectX::XMLoadFloat3A(&v_)) {}
	[[nodiscard]]
	explicit Vec3A(const Vec3& v_) noexcept 
		: v(DirectX::XMLoadFloat3(&v_)) {}

	[[nodiscard]]
	constexpr explicit Vec3A(DirectX::XMVECTOR v_) noexcept : v(v_) {}

	[[nodiscard]]
	operator DirectX::XMFLOAT3A() const noexcept
	{
		DirectX::XMFLOAT3A result;
		DirectX::XMStoreFloat3A(&result, v);
		return result;
	}

	[[nodiscard]] DirectX::XMVECTOR Vector() const noexcept { return v; }

	[[nodiscard]] Vec3 ToVec3() const noexcept { Vec3 temp; DirectX::XMStoreFloat3(&temp, this->v); return temp; }

	[[nodiscard]] DirectX::XMFLOAT3A ToFloat3A() const noexcept
	{
		DirectX::XMFLOAT3A out;
		DirectX::XMStoreFloat3A(&out, v);
		return out;
	}

	[[nodiscard]] static Vec3A FromFloat3A(const DirectX::XMFLOAT3A& f) noexcept 
	{
		return static_cast<Vec3A>(DirectX::XMLoadFloat3A(&f));
	}
#pragma endregion

#pragma region Operator
	Vec3A operator+(const Vec3A& rhs) const noexcept
	{
		return static_cast<Vec3A>(DirectX::XMVectorAdd(v, rhs.v));
	}

	[[nodiscard]]
	Vec3A operator-(const Vec3A& rhs) const noexcept
	{
		return Vec3A(DirectX::XMVectorSubtract(v, rhs.v));
	}

	[[nodiscard]]
	Vec3A operator*(const Vec3A& rhs) const noexcept
	{
		return Vec3A(DirectX::XMVectorMultiply(this->v, rhs.v));
	}
	[[nodiscard]]
	Vec3A operator/(const Vec3A& rhs) const noexcept
	{
		return Vec3A(DirectX::XMVectorDivide(this->v, rhs.v));
	}
	[[nodiscard]]
	Vec3A operator*(float scalar) const noexcept
	{
		return Vec3A(DirectX::XMVectorScale(v, scalar));
	}
	[[nodiscard]]
	Vec3A operator/(float scalar) const noexcept
	{
		return Vec3A(DirectX::XMVectorScale(v, 1.0f / scalar));
	}

	Vec3A& operator+=(const Vec3A& rhs) noexcept 
	{ 
		v = DirectX::XMVectorAdd(v, rhs.v); return *this; 
	}
	Vec3A& operator-=(const Vec3A& rhs) noexcept 
	{ 
		v = DirectX::XMVectorSubtract(v, rhs.v); return *this; 
	}
	Vec3A& operator*=(float scalar) noexcept 
	{ 
		v = DirectX::XMVectorScale(v, scalar); return *this; 
	}
	Vec3A& operator/=(float scalar) noexcept 
	{
		assert(scalar != 0.0f);
		v = DirectX::XMVectorScale(v, 1.0f / scalar); return *this; 
	}

	[[nodiscard]]
	Vec3A operator-() const noexcept { return *this * -1.0f; }

	[[nodiscard]]
	bool NearEqual(const Vec3A& rhs, float epsilon = 1e-6f) const noexcept
	{
		return DirectX::XMVector3NearEqual(v, rhs.v, DirectX::XMVectorReplicate(epsilon));
	}
#pragma endregion

#pragma region Utility
	float x() const noexcept { return DirectX::XMVectorGetX(v); }
	float y() const noexcept { return DirectX::XMVectorGetY(v); }
	float z() const noexcept { return DirectX::XMVectorGetZ(v); }

	[[nodiscard]] float Length() const noexcept 
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Length(v));
	}
	[[nodiscard]] Vec3A Normalize() const noexcept 
	{
		return Vec3A(DirectX::XMVector3Normalize(v));
	}
	[[nodiscard]] float Dot(const Vec3A& rhs) const noexcept
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(v, rhs.v));
	}
	[[nodiscard]] Vec3A Cross(const Vec3A& rhs) const noexcept 
	{
		return Vec3A(DirectX::XMVector3Cross(v, rhs.v));
	}
#pragma endregion

#pragma region Static
	[[nodiscard]] static constexpr Vec3A Zero() noexcept 
	{
		return Vec3A(DirectX::XMVectorZero());
	}
	[[nodiscard]] static Vec3A One() noexcept 
	{
		return Vec3A{ 1.0f, 1.0f, 1.0f };
	}
	[[nodiscard]] static Vec3A Right() noexcept 
	{
		return Vec3A{ 1.0f, 0.0f, 0.0f };
	}
	[[nodiscard]] static Vec3A Up() noexcept
	{
		return Vec3A{ 0.0f, 1.0f, 0.0f };
	}
	[[nodiscard]] static Vec3A Forward() noexcept
	{
		return Vec3A{ 0.0f, 0.0f, 1.0f };
	}
#pragma endregion

};