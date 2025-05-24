#pragma once

struct Vec4
{
#pragma region Constructor

#pragma endregion

#pragma region Operator

#pragma endregion

#pragma region Utility

#pragma endregion

#pragma region Static

#pragma endregion
	float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

	Vec4() = default;
	Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

	Vec4 operator+(const Vec4& rhs) const { return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	Vec4 operator-(const Vec4& rhs) const { return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	Vec4 operator*(float scalar) const { return Vec4(x * scalar, y * scalar, z * scalar, w * scalar); }
	Vec4 operator/(float scalar) const { return Vec4(x / scalar, y / scalar, z / scalar, w / scalar); }

	bool operator==(const Vec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=(const Vec4& rhs) const { return !(*this == rhs); }

	Vec4(const DirectX::XMFLOAT4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
	operator DirectX::XMFLOAT4() const { return DirectX::XMFLOAT4(x, y, z, w); }

	operator DirectX::XMVECTOR() const { return DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(this)); }

};
