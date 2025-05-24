#pragma once

struct Int2 {
	int32_t x = 0, y = 0;

#pragma region Constructor
	constexpr Int2() noexcept = default;
	constexpr Int2(int32_t x_, int32_t y_) noexcept : x(x_), y(y_) {}

	[[nodiscard]]
	explicit Int2(const DirectX::XMFLOAT2& v) noexcept
		: x(static_cast<int32_t>(v.x)), y(static_cast<int32_t>(v.y)) {
	}

	[[nodiscard]]
	constexpr operator DirectX::XMFLOAT2() const noexcept {
		return DirectX::XMFLOAT2(float(x), float(y));
	}
#pragma endregion

#pragma region Operator
	[[nodiscard]]
	constexpr Int2 operator+(const Int2& rhs) const noexcept { return { x + rhs.x, y + rhs.y }; }
	[[nodiscard]]
	constexpr Int2 operator-(const Int2& rhs) const noexcept { return { x - rhs.x, y - rhs.y }; }
	[[nodiscard]]
	constexpr Int2 operator*(const Int2& rhs) const noexcept { return { x * rhs.x, y * rhs.y }; }
	[[nodiscard]]
	constexpr Int2 operator/(const Int2& rhs) const noexcept { return { x / rhs.x, y / rhs.y }; }
	[[nodiscard]]
	constexpr Int2 operator*(int32_t scalar) const noexcept { return { x * scalar, y * scalar }; }
	[[nodiscard]]
	constexpr Int2 operator/(int32_t scalar) const noexcept { return { x / scalar, y / scalar }; }

	Int2& operator+=(const Int2& rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
	Int2& operator-=(const Int2& rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }
	Int2& operator*=(const Int2& rhs) noexcept { x *= rhs.x; y *= rhs.y; return *this; }
	Int2& operator/=(const Int2& rhs) noexcept { x /= rhs.x; y /= rhs.y; return *this; }
	Int2& operator*=(int32_t scalar) noexcept { x *= scalar; y *= scalar; return *this; }
	Int2& operator/=(int32_t scalar) noexcept { x /= scalar; y /= scalar; return *this; }

	[[nodiscard]]
	constexpr Int2 operator-() const noexcept { return { -x, -y }; }
	[[nodiscard]]
	constexpr bool operator==(const Int2& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
	[[nodiscard]]
	constexpr bool operator!=(const Int2& rhs) const noexcept { return !(*this == rhs); }
#pragma endregion

#pragma region Utility
	[[nodiscard]]
	static constexpr Int2 Min(const Int2& a, const Int2& b) noexcept {
		return { std::min(a.x, b.x), std::min(a.y, b.y) };
	}
	[[nodiscard]]
	static constexpr Int2 Max(const Int2& a, const Int2& b) noexcept {
		return { std::max(a.x, b.x), std::max(a.y, b.y) };
	}
	[[nodiscard]]
	static constexpr Int2 Clamp(const Int2& v, const Int2& vmin, const Int2& vmax) noexcept {
		return Max(vmin, Min(v, vmax));
	}
#pragma endregion

#pragma region Static
	[[nodiscard]] static consteval Int2 Zero() noexcept { return { 0, 0 }; }
	[[nodiscard]] static consteval Int2 One() noexcept { return { 1, 1 }; }
	[[nodiscard]] static consteval Int2 UnitX() noexcept { return { 1, 0 }; }
	[[nodiscard]] static consteval Int2 UnitY() noexcept { return { 0, 1 }; }
#pragma endregion

};

struct Int3 {
	int32_t x = 0, y = 0, z = 0;

#pragma region Constructor
	constexpr Int3() noexcept = default;
	constexpr Int3(int32_t x_, int32_t y_, int32_t z_) noexcept : x(x_), y(y_), z(z_) {}

	[[nodiscard]]
	explicit Int3(const DirectX::XMFLOAT3& v) noexcept
		: x(static_cast<int32_t>(v.x)), y(static_cast<int32_t>(v.y)), z(static_cast<int32_t>(v.z)) {
	}

	[[nodiscard]]
	constexpr operator DirectX::XMFLOAT3() const noexcept {
		return DirectX::XMFLOAT3(float(x), float(y), float(z));
	}
#pragma endregion

#pragma region Operator
	[[nodiscard]]
	constexpr Int3 operator+(const Int3& rhs) const noexcept { return { x + rhs.x, y + rhs.y, z + rhs.z }; }
	[[nodiscard]]
	constexpr Int3 operator-(const Int3& rhs) const noexcept { return { x - rhs.x, y - rhs.y, z - rhs.z }; }
	[[nodiscard]]
	constexpr Int3 operator*(const Int3& rhs) const noexcept { return { x * rhs.x, y * rhs.y, z * rhs.z }; }
	[[nodiscard]]
	constexpr Int3 operator/(const Int3& rhs) const noexcept { return { x / rhs.x, y / rhs.y, z / rhs.z }; }
	[[nodiscard]]
	constexpr Int3 operator*(int32_t scalar) const noexcept { return { x * scalar, y * scalar, z * scalar }; }
	[[nodiscard]]
	constexpr Int3 operator/(int32_t scalar) const noexcept { return { x / scalar, y / scalar, z / scalar }; }

	Int3& operator+=(const Int3& rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	Int3& operator-=(const Int3& rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	Int3& operator*=(const Int3& rhs) noexcept { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
	Int3& operator/=(const Int3& rhs) noexcept { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }
	Int3& operator*=(int32_t scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
	Int3& operator/=(int32_t scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; return *this; }

	[[nodiscard]]
	constexpr Int3 operator-() const noexcept { return { -x, -y, -z }; }
	[[nodiscard]]
	constexpr bool operator==(const Int3& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z; }
	[[nodiscard]]
	constexpr bool operator!=(const Int3& rhs) const noexcept { return !(*this == rhs); }
#pragma endregion

#pragma region Utility
	[[nodiscard]]
	static constexpr Int3 Min(const Int3& a, const Int3& b) noexcept {
		return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
	}
	[[nodiscard]]
	static constexpr Int3 Max(const Int3& a, const Int3& b) noexcept {
		return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
	}
	[[nodiscard]]
	static constexpr Int3 Clamp(const Int3& v, const Int3& vmin, const Int3& vmax) noexcept {
		return Max(vmin, Min(v, vmax));
	}
#pragma endregion

#pragma region Static
	[[nodiscard]] static consteval Int3 Zero() noexcept { return { 0, 0, 0 }; }
	[[nodiscard]] static consteval Int3 One() noexcept { return { 1, 1, 1 }; }
	[[nodiscard]] static consteval Int3 UnitX() noexcept { return { 1, 0, 0 }; }
	[[nodiscard]] static consteval Int3 UnitY() noexcept { return { 0, 1, 0 }; }
	[[nodiscard]] static consteval Int3 UnitZ() noexcept { return { 0, 0, 1 }; }
#pragma endregion
};