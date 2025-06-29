#pragma once

struct Vec2
{
	float x = 0.0f, y = 0.0f;

#pragma region Constructor
	constexpr Vec2() noexcept = default;
	constexpr Vec2(float x_, float y_) noexcept : x(x_), y(y_) {}

	[[nodiscard]]
	explicit Vec2(const DirectX::XMFLOAT2& v) noexcept : x(v.x), y(v.y) {}

	[[nodiscard]]
	constexpr operator DirectX::XMFLOAT2() const noexcept { return DirectX::XMFLOAT2(x, y); }
#pragma endregion

#pragma region Operator
	[[nodiscard]]
	constexpr Vec2 operator+(const Vec2& rhs) const noexcept { return { x + rhs.x, y + rhs.y }; }
	[[nodiscard]]
	constexpr Vec2 operator-(const Vec2& rhs) const noexcept { return { x - rhs.x, y - rhs.y }; }
	[[nodiscard]]
	constexpr Vec2 operator*(const Vec2& rhs) const noexcept { return { x * rhs.x, y * rhs.y }; }
	[[nodiscard]]
	constexpr Vec2 operator/(const Vec2& rhs) const noexcept { return { x / rhs.x, y / rhs.y }; }
	[[nodiscard]]
	constexpr Vec2 operator*(float scalar) const noexcept { return { x * scalar, y * scalar }; }
	[[nodiscard]]
	constexpr Vec2 operator/(float scalar) const noexcept { return { x / scalar, y / scalar }; }

	Vec2& operator+=(const Vec2& rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
	Vec2& operator-=(const Vec2& rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }
	Vec2& operator*=(const Vec2& rhs) noexcept { x *= rhs.x; y *= rhs.y; return *this; }
	Vec2& operator/=(const Vec2& rhs) noexcept { x /= rhs.x; y /= rhs.y; return *this; }
	Vec2& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; return *this; }
	Vec2& operator/=(float scalar) noexcept { x /= scalar; y /= scalar; return *this; }

	[[nodiscard]]
	constexpr Vec2 operator-() const noexcept { return { -x, -y }; }
	[[nodiscard]]
	bool operator==(const Vec2& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
	[[nodiscard]]
	bool operator!=(const Vec2& rhs) const noexcept { return !(*this == rhs); }
#pragma endregion

#pragma region Utility
	[[nodiscard]]
	constexpr float LengthSquared() const noexcept { return x * x + y * y; }
	[[nodiscard]]
	float Length() const noexcept { return std::sqrt(x * x + y * y); }

	void Normalize() noexcept {
		float len = Length();
		if (len > 0) { x /= len; y /= len; }
	}
	[[nodiscard]]
	Vec2 Normalized() const noexcept {
		float len = Length();
		return (len > 0) ? Vec2(x / len, y / len) : Vec2(0, 0);
	}
	[[nodiscard]]
	constexpr float Cross(const Vec2& rhs) const noexcept { return x * rhs.y - y * rhs.x; }
	[[nodiscard]]
	constexpr float Dot(const Vec2& rhs) const noexcept { return x * rhs.x + y * rhs.y; }

	[[nodiscard]]
	static constexpr Vec2 Min(const Vec2& a, const Vec2& b) noexcept {
		return { std::min(a.x, b.x), std::min(a.y, b.y) };
	}
	[[nodiscard]]
	static constexpr Vec2 Max(const Vec2& a, const Vec2& b) noexcept {
		return { std::max(a.x, b.x), std::max(a.y, b.y) };
	}
	[[nodiscard]]
	static constexpr Vec2 Clamp(const Vec2& v, const Vec2& vmin, const Vec2& vmax) noexcept {
		return Max(vmin, Min(v, vmax));
	}
	[[nodiscard]]
	static constexpr Vec2 Lerp(const Vec2& a, const Vec2& b, float t) noexcept {
		return Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
	}
#pragma endregion

#pragma region Static
	[[nodiscard]] static consteval Vec2 Zero() noexcept { return { 0.0f, 0.0f }; }
	[[nodiscard]] static consteval Vec2 One() noexcept { return { 1.0f, 1.0f }; }
	[[nodiscard]] static consteval Vec2 UnitX() noexcept { return { 1.0f, 0.0f }; }
	[[nodiscard]] static consteval Vec2 UnitY() noexcept { return { 0.0f, 1.0f }; }
#pragma endregion
};
