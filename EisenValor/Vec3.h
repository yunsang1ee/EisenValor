#pragma once

struct Vec3
{
	float x = 0.0f, y = 0.0f, z = 0.0f;

#pragma region Constructor
	constexpr Vec3() noexcept = default;
	constexpr Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

	[[nodiscard]]
	constexpr explicit Vec3(const DirectX::XMFLOAT3& v) noexcept : x(v.x), y(v.y), z(v.z) {}
	
	[[nodiscard]]
	constexpr operator DirectX::XMFLOAT3() const noexcept { return DirectX::XMFLOAT3(x, y, z); }

#pragma endregion

#pragma region Operator
    Vec3 operator+(const Vec3& rhs) const noexcept 
    {
       using namespace DirectX;
	   XMFLOAT3 val1 = static_cast<XMFLOAT3>(*this);
	   XMFLOAT3 val2 = static_cast<XMFLOAT3>(rhs);
       XMVECTOR v1 = XMLoadFloat3(&val1);
       XMVECTOR v2 = XMLoadFloat3(&val2);
       XMFLOAT3 resultFloat3;
       XMStoreFloat3(&resultFloat3, v1 + v2);
       return Vec3(resultFloat3);
    }
	[[nodiscard]]
	Vec3 operator-(const Vec3& rhs) const noexcept 
	{
		using namespace DirectX;
		XMFLOAT3 val1 = static_cast<XMFLOAT3>(*this);
		XMFLOAT3 val2 = static_cast<XMFLOAT3>(rhs);
		XMVECTOR v1 = XMLoadFloat3(&val1);
		XMVECTOR v2 = XMLoadFloat3(&val2);
		XMFLOAT3 resultFloat3;
		XMStoreFloat3(&resultFloat3, v1 - v2);
		return Vec3(resultFloat3);
	}
	[[nodiscard]]
	Vec3 operator*(const Vec3& rhs) const noexcept 
	{
		using namespace DirectX;
		XMFLOAT3 val1 = static_cast<XMFLOAT3>(*this);
		XMFLOAT3 val2 = static_cast<XMFLOAT3>(rhs);
		XMVECTOR v1 = XMLoadFloat3(&val1);
		XMVECTOR v2 = XMLoadFloat3(&val2);
		XMFLOAT3 resultFloat3;
		XMStoreFloat3(&resultFloat3, v1 * v2);
		return Vec3(resultFloat3);
	}
	[[nodiscard]]
	Vec3 operator/(const Vec3& rhs) const noexcept 
	{
		using namespace DirectX;
		XMFLOAT3 val1 = static_cast<XMFLOAT3>(*this);
		XMFLOAT3 val2 = static_cast<XMFLOAT3>(rhs);
		XMVECTOR v1 = XMLoadFloat3(&val1);
		XMVECTOR v2 = XMLoadFloat3(&val2);
		XMFLOAT3 resultFloat3;
		XMStoreFloat3(&resultFloat3, v1 / v2);
		return Vec3(resultFloat3);
	}
	[[nodiscard]]
	Vec3 operator*(float scalar) const noexcept 
	{
		using namespace DirectX;
		XMFLOAT3 val1 = static_cast<XMFLOAT3>(*this);
		XMVECTOR v1 = XMLoadFloat3(&val1);
		XMFLOAT3 resultFloat3;
		XMStoreFloat3(&resultFloat3, XMVectorScale(v1, scalar));
		return Vec3(resultFloat3);
	}
	[[nodiscard]]
	Vec3 operator/(float scalar) const noexcept 
	{
		using namespace DirectX;
		XMFLOAT3 val1 = static_cast<XMFLOAT3>(*this);
		XMVECTOR v1 = XMLoadFloat3(&val1);
		XMFLOAT3 resultFloat3;
		XMVECTOR vS = XMVectorReplicate(scalar);
		XMStoreFloat3(&resultFloat3, XMVectorDivide(v1, vS));
		return Vec3(resultFloat3);
	}

    Vec3& operator+=(const Vec3& rhs) noexcept 
    {
       return *this = *this + rhs;
    }
	Vec3& operator-=(const Vec3& rhs) noexcept 
	{
		return *this = *this - rhs;
	}
	Vec3& operator*=(const Vec3& rhs) noexcept 
	{
		return *this = *this * rhs;
	}
	Vec3& operator/=(const Vec3& rhs) noexcept 
	{
		return *this = *this / rhs;
	}
	Vec3& operator*=(float scalar) noexcept 
	{
		return *this = *this * scalar;
	}
	Vec3& operator/=(float scalar) noexcept 
	{
		return *this = *this / scalar;
	}

	[[nodiscard]]
	Vec3 operator-() const noexcept { return *this * -1.0f; }
	[[nodiscard]]
	bool operator==(const Vec3& rhs) const noexcept 
	{
		using namespace DirectX;
		XMFLOAT3 val1 = static_cast<XMFLOAT3>(*this);
		XMFLOAT3 val2 = static_cast<XMFLOAT3>(rhs);
		XMVECTOR v1 = XMLoadFloat3(&val1);
		XMVECTOR v2 = XMLoadFloat3(&val2);
		return XMVector3Equal(v1, v2);
	}
#pragma endregion

#pragma region Utility
	[[nodiscard]] constexpr float LengthSquared() const noexcept { return x * x + y * y + z * z; }
	[[nodiscard]] float Length() const noexcept { return std::sqrt(x * x + y * y + z * z); }

	void Normalize() noexcept {
		float len = Length();
		if (len > 0) { x /= len; y /= len; z /= len; }
	}
	[[nodiscard]] Vec3 Normalized() const noexcept {
		float len = Length();
		return (len > 0) ? Vec3(x / len, y / len, z / len) : Vec3(0, 0, 0);
	}

	[[nodiscard]] constexpr float Dot(const Vec3& rhs) const noexcept {
		return x * rhs.x + y * rhs.y + z * rhs.z;
	}

	[[nodiscard]] Vec3 Cross(const Vec3& rhs) const noexcept {
		return Vec3(
			y * rhs.z - z * rhs.y,
			z * rhs.x - x * rhs.z,
			x * rhs.y - y * rhs.x
		);
	}

	[[nodiscard]]
	static Vec3 XMNormalized(const Vec3& v) noexcept {
		DirectX::XMFLOAT3 xf(v.x, v.y, v.z), result;
		DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&xf)));
		return Vec3(result);
	}

	[[nodiscard]]
	static Vec3 XMCross(const Vec3& a, const Vec3& b, bool isOutNormalize = false) noexcept {
		DirectX::XMFLOAT3 af(a.x, a.y, a.z), bf(b.x, b.y, b.z), result;
		auto v0 = DirectX::XMLoadFloat3(&af);
		auto v1 = DirectX::XMLoadFloat3(&bf);
		auto cross = DirectX::XMVector3Cross(v0, v1);
		if (isOutNormalize)
			cross = DirectX::XMVector3Normalize(cross);
		DirectX::XMStoreFloat3(&result, cross);
		return Vec3(result);
	}

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
#pragma endregion

#pragma region Static
	[[nodiscard]] static consteval Vec3 Zero() noexcept { return { 0.0f, 0.0f, 0.0f }; }
	[[nodiscard]] static consteval Vec3 One() noexcept { return { 1.0f, 1.0f, 1.0f }; }
	[[nodiscard]] static consteval Vec3 UnitX() noexcept { return { 1.0f, 0.0f, 0.0f }; }
	[[nodiscard]] static consteval Vec3 UnitY() noexcept { return { 0.0f, 1.0f, 0.0f }; }
	[[nodiscard]] static consteval Vec3 UnitZ() noexcept { return { 0.0f, 0.0f, 1.0f }; }
#pragma endregion
};


// -----------------------------------------------
// Aligned Vec3
// -----------------------------------------------

struct alignas(16) Vec3A
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	[[maybe_unused]] float _padding;

#pragma region Constructor
	constexpr Vec3A() noexcept = default;
	constexpr Vec3A(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

	[[nodiscard]]
	constexpr explicit Vec3A(const DirectX::XMFLOAT3A& v) noexcept : x(v.x), y(v.y), z(v.z) {}

	[[nodiscard]]
	constexpr operator DirectX::XMFLOAT3A() const noexcept { return DirectX::XMFLOAT3A(x, y, z); }

#pragma endregion

#pragma region Operator
	Vec3A operator+(const Vec3A& rhs) const noexcept
	{
		using namespace DirectX;
		XMFLOAT3A val1 = static_cast<XMFLOAT3A>(*this);
		XMFLOAT3A val2 = static_cast<XMFLOAT3A>(rhs);
		XMVECTOR v1 = XMLoadFloat3A(&val1);
		XMVECTOR v2 = XMLoadFloat3A(&val2);
		XMFLOAT3A resultFloat3;
		XMStoreFloat3A(&resultFloat3, v1 + v2);
		return Vec3A(resultFloat3);
	}
	[[nodiscard]]
	Vec3A operator-(const Vec3A& rhs) const noexcept
	{
		using namespace DirectX;
		XMFLOAT3A val1 = static_cast<XMFLOAT3A>(*this);
		XMFLOAT3A val2 = static_cast<XMFLOAT3A>(rhs);
		XMVECTOR v1 = XMLoadFloat3A(&val1);
		XMVECTOR v2 = XMLoadFloat3A(&val2);
		XMFLOAT3A resultFloat3;
		XMStoreFloat3A(&resultFloat3, v1 - v2);
		return Vec3A(resultFloat3);
	}
	[[nodiscard]]
	Vec3A operator*(const Vec3A& rhs) const noexcept
	{
		using namespace DirectX;
		XMFLOAT3A val1 = static_cast<XMFLOAT3A>(*this);
		XMFLOAT3A val2 = static_cast<XMFLOAT3A>(rhs);
		XMVECTOR v1 = XMLoadFloat3A(&val1);
		XMVECTOR v2 = XMLoadFloat3A(&val2);
		XMFLOAT3A resultFloat3;
		XMStoreFloat3A(&resultFloat3, v1 * v2);
		return Vec3A(resultFloat3);
	}
	[[nodiscard]]
	Vec3A operator/(const Vec3A& rhs) const noexcept
	{
		using namespace DirectX;
		XMFLOAT3A val1 = static_cast<XMFLOAT3A>(*this);
		XMFLOAT3A val2 = static_cast<XMFLOAT3A>(rhs);
		XMVECTOR v1 = XMLoadFloat3A(&val1);
		XMVECTOR v2 = XMLoadFloat3A(&val2);
		XMFLOAT3A resultFloat3;
		XMStoreFloat3A(&resultFloat3, v1 / v2);
		return Vec3A(resultFloat3);
	}
	[[nodiscard]]
	Vec3A operator*(float scalar) const noexcept
	{
		using namespace DirectX;
		XMFLOAT3A val1 = static_cast<XMFLOAT3A>(*this);
		XMVECTOR v1 = XMLoadFloat3A(&val1);
		XMFLOAT3A resultFloat3;
		XMStoreFloat3A(&resultFloat3, v1 * scalar);
		return Vec3A(resultFloat3);
	}
	[[nodiscard]]
	Vec3A operator/(float scalar) const noexcept
	{
		using namespace DirectX;
		XMFLOAT3A val1 = static_cast<XMFLOAT3A>(*this);
		XMVECTOR v1 = XMLoadFloat3A(&val1);
		XMFLOAT3A resultFloat3;
		XMStoreFloat3A(&resultFloat3, v1 / scalar);
		return Vec3A(resultFloat3);
	}

	Vec3A& operator+=(const Vec3A& rhs) noexcept
	{
		return *this = *this + rhs;
	}
	Vec3A& operator-=(const Vec3A& rhs) noexcept
	{
		return *this = *this - rhs;
	}
	Vec3A& operator*=(const Vec3A& rhs) noexcept
	{
		return *this = *this * rhs;
	}
	Vec3A& operator/=(const Vec3A& rhs) noexcept
	{
		return *this = *this / rhs;
	}
	Vec3A& operator*=(float scalar) noexcept
	{
		return *this = *this * scalar;
	}
	Vec3A& operator/=(float scalar) noexcept
	{
		return *this = *this / scalar;
	}

	[[nodiscard]]
	Vec3A operator-() const noexcept { return *this * -1.0f; }
	[[nodiscard]]
	bool operator==(const Vec3A& rhs) const noexcept
	{
		using namespace DirectX;
		XMFLOAT3A val1 = static_cast<XMFLOAT3A>(*this);
		XMFLOAT3A val2 = static_cast<XMFLOAT3A>(rhs);
		XMVECTOR v1 = XMLoadFloat3A(&val1);
		XMVECTOR v2 = XMLoadFloat3A(&val2);
		return XMVector3Equal(v1, v2);
	}
#pragma endregion

};