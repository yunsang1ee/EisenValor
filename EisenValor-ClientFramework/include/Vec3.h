#pragma once

struct Vec3
{
	float x = 0.0f, y = 0.0f, z = 0.0f;

#pragma region Constructor & Translator
	constexpr Vec3() noexcept = default;
	constexpr Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

	[[nodiscard]]
	constexpr explicit Vec3(const DirectX::XMFLOAT3& v) noexcept : x(v.x), y(v.y), z(v.z) {}
	
	[[nodiscard]]
	constexpr operator DirectX::XMFLOAT3() const noexcept { return DirectX::XMFLOAT3(x, y, z); }

	[[nodiscard]]
	DirectX::XMVECTOR ToVector() const noexcept
	{
		return DirectX::XMVectorSet(x, y, z, 0.0f);
	}
	[[nodiscard]]
	static Vec3 FromVector(DirectX::XMVECTOR v) noexcept
	{
		DirectX::XMFLOAT3 out;
		DirectX::XMStoreFloat3(&out, v);
		return static_cast<Vec3>(out);
	}
#pragma endregion

#pragma region Static
	[[nodiscard]] static consteval Vec3 Zero() noexcept 
	{
		return { 0.0f, 0.0f, 0.0f };
	}
	[[nodiscard]] static consteval Vec3 One() noexcept 
	{
		return { 1.0f, 1.0f, 1.0f };
	}
	[[nodiscard]] static consteval Vec3 Right() noexcept 
	{
		return { 1.0f, 0.0f, 0.0f };
	}
	[[nodiscard]] static consteval Vec3 Up() noexcept 
	{
		return { 0.0f, 1.0f, 0.0f };
	}
	[[nodiscard]] static consteval Vec3 Forward() noexcept 
	{
		return { 0.0f, 0.0f, 1.0f };
	}
#pragma endregion
};


// -----------------------------------------------
// Aligned Vec3
// -----------------------------------------------

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
		: v(v_.ToVector()) {}

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

	[[nodiscard]] Vec3 ToVec3() const noexcept { return Vec3::FromVector(v); }

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