#pragma once
#include "stdafxClientFramework.h"
#include "Vec3.h"

using XVec2 = DX::XMFLOAT2;
using XVec3 = DX::XMFLOAT3;
using XVec3A = DX::XMFLOAT3A;
using XVec4 = DX::XMFLOAT4;
using XMat4x4 = DX::XMFLOAT4X4;
#define LoadVec2 DX::XMLoadFloat2
#define LoadVec3 DX::XMLoadFloat3
#define LoadVec3A DX::XMLoadFloat3A
#define LoadVec4 DX::XMLoadFloat4


namespace Math
{
template <typename T>
concept VecNA = requires(T v) {
	typename T::dimension_tag;
	requires std::is_base_of_v<std::integral_constant<int, T::dimension_tag::value>, typename T::dimension_tag>;
	{ v.Vector() } -> std::same_as<DirectX::XMVECTOR>;
};
} // namespace Math

namespace Math::detail
{
#pragma region NearEqual
template <VecNA Vec, typename Tag>
struct NearEqualDispatcher;

template <VecNA Vec>
struct NearEqualDispatcher<Vec, std::integral_constant<int, 2>>
{
	static bool call(const Vec& a, const Vec& b, float epsilon)
	{
		return DirectX::XMVector2NearEqual(a.Vector(), b.Vector(), DirectX::XMVectorReplicate(epsilon));
	}
};

template <VecNA Vec>
struct NearEqualDispatcher<Vec, std::integral_constant<int, 3>>
{
	static bool call(const Vec& a, const Vec& b, float epsilon)
	{
		return DirectX::XMVector3NearEqual(a.Vector(), b.Vector(), DirectX::XMVectorReplicate(epsilon));
	}
};

template <VecNA Vec>
struct NearEqualDispatcher<Vec, std::integral_constant<int, 4>>
{
	static bool call(const Vec& a, const Vec& b, float epsilon)
	{
		return DirectX::XMVector4NearEqual(a.Vector(), b.Vector(), DirectX::XMVectorReplicate(epsilon));
	}
};
#pragma endregion

} // namespace Math::detail

namespace Math
{
template <VecNA Vec>
[[nodiscard]]
inline bool NearEqual(const Vec& a, const Vec& b, float epsilon = 1e-6f)
{
	return detail::NearEqualDispatcher<Vec, Vec::dimension_tag>::call(a, b, epsilon);
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

inline Int2 XMConvertToInt2(const XMFLOAT2& f
return Int2(int32_t(f.x), int32_t(f.y)); }
inline XMFLOAT2 XMConvertToXMFLOAT2(const Int2& i) { return XMFLOAT2(float(i.x), float(i.y)); }
*/
} // namespace Math

inline uint32_t RandomColor()
{
	static std::mt19937							   rng{std::random_device{}()};
	static std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFF);
	return 0xFF000000 | dist(rng);
}
consteval float DegreeToRadian(float degrees)
{
	return degrees * 3.141592654f / 180.0f;
}

constexpr auto EPSILON = 1.0e-6f;

inline bool IsZero(float fValue)
{
	return ((fabsf(fValue) < EPSILON));
}
inline bool IsEqual(float fA, float fB)
{
	return (::IsZero(fA - fB));
}

namespace Vector3
{
using namespace DirectX;

inline XMFLOAT3 XMVectorToFloat3(const XMVECTOR& xmvVector)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, xmvVector);
	return (xmf3Result);
}

inline XMFLOAT3 ScalarProduct(const XMFLOAT3& xmf3Vector, float fScalar, bool bNormalize = true)
{
	XMFLOAT3 xmf3Result;
	if (bNormalize)
		XMStoreFloat3(&xmf3Result, XMVector3Normalize(XMLoadFloat3(&xmf3Vector)) * fScalar);
	else
		XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector) * fScalar);
	return (xmf3Result);
}

inline XMFLOAT3 Add(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector1) + XMLoadFloat3(&xmf3Vector2));
	return (xmf3Result);
}

inline XMFLOAT3 Add(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2, float fScalar)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector1) + (XMLoadFloat3(&xmf3Vector2) * fScalar));
	return (xmf3Result);
}

inline XMFLOAT3 Subtract(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector1) - XMLoadFloat3(&xmf3Vector2));
	return (xmf3Result);
}

inline float DotProduct(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, XMVector3Dot(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
	return (xmf3Result.x);
}

inline XMFLOAT3 CrossProduct(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2, bool bNormalize = true)
{
	XMFLOAT3 xmf3Result;
	if (bNormalize)
		XMStoreFloat3(
			&xmf3Result,
			XMVector3Normalize(XMVector3Cross(
				XMVector3Normalize(XMLoadFloat3(&xmf3Vector1)), XMVector3Normalize(XMLoadFloat3(&xmf3Vector2))
			))
		);
	else
		XMStoreFloat3(
			&xmf3Result,
			XMVector3Cross(
				XMVector3Normalize(XMLoadFloat3(&xmf3Vector1)), XMVector3Normalize(XMLoadFloat3(&xmf3Vector2))
			)
		);
	return (xmf3Result);
}

inline XMFLOAT3 Normalize(const XMFLOAT3& xmf3Vector)
{
	XMFLOAT3 xmf3Normal;
	XMStoreFloat3(&xmf3Normal, XMVector3Normalize(XMLoadFloat3(&xmf3Vector)));
	return (xmf3Normal);
}

inline float Length(const XMFLOAT3& xmf3Vector)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, XMVector3Length(XMLoadFloat3(&xmf3Vector)));
	return (xmf3Result.x);
}

inline float Distance(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(
		&xmf3Result, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)))
	);
	return (xmf3Result.x);
}

inline float Angle(const XMVECTOR& xmvVector1, const XMVECTOR& xmvVector2)
{
	XMVECTOR xmvAngle = XMVector3AngleBetweenNormals(xmvVector1, xmvVector2);
	return (XMVectorGetX(XMVectorACos(xmvAngle)));
}

inline float Angle(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2)
{
	return (Angle(XMVector3Normalize(XMLoadFloat3(&xmf3Vector1)), XMVector3Normalize(XMLoadFloat3(&xmf3Vector2))));
}

inline XMFLOAT3 TransformNormal(const XMFLOAT3& xmf3Vector, const XMMATRIX& xmxm4x4Transform)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, XMVector3TransformNormal(XMLoadFloat3(&xmf3Vector), xmxm4x4Transform));
	return (xmf3Result);
}

inline XMFLOAT3 TransformNormal(const XMFLOAT3& xmf3Vector, const XMFLOAT4X4& xmmtx4x4Matrix)
{
	return (TransformNormal(xmf3Vector, XMLoadFloat4x4(&xmmtx4x4Matrix)));
}

inline XMFLOAT3 TransformCoord(const XMFLOAT3& xmf3Vector, const XMMATRIX& xmxm4x4Transform)
{
	XMFLOAT3 xmf3Result;
	XMStoreFloat3(&xmf3Result, XMVector3TransformCoord(XMLoadFloat3(&xmf3Vector), xmxm4x4Transform));
	return (xmf3Result);
}

inline XMFLOAT3 TransformCoord(const XMFLOAT3& xmf3Vector, const XMFLOAT4X4& xmmtx4x4Matrix)
{
	return (TransformCoord(xmf3Vector, XMLoadFloat4x4(&xmmtx4x4Matrix)));
}
} // namespace Vector3

namespace Vector4
{
using namespace DirectX;
inline XMFLOAT4 Add(const XMFLOAT4& xmf4Vector1, const XMFLOAT4& xmf4Vector2)
{
	XMFLOAT4 xmf4Result;
	XMStoreFloat4(&xmf4Result, XMLoadFloat4(&xmf4Vector1) + XMLoadFloat4(&xmf4Vector2));
	return (xmf4Result);
}
} // namespace Vector4

namespace Matrix4x4
{
using namespace DirectX;
inline XMFLOAT4X4 Identity()
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, XMMatrixIdentity());
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 Translate(float x, float y, float z)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, XMMatrixTranslation(x, y, z));
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 Multiply(const XMFLOAT4X4& xmmtx4x4Matrix1, const XMFLOAT4X4& xmmtx4x4Matrix2)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, XMLoadFloat4x4(&xmmtx4x4Matrix1) * XMLoadFloat4x4(&xmmtx4x4Matrix2));
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 Multiply(const XMFLOAT4X4& xmmtx4x4Matrix1, const XMMATRIX& xmmtxMatrix2)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, XMLoadFloat4x4(&xmmtx4x4Matrix1) * xmmtxMatrix2);
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 Multiply(const XMMATRIX& xmmtxMatrix1, const XMFLOAT4X4& xmmtx4x4Matrix2)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, xmmtxMatrix1 * XMLoadFloat4x4(&xmmtx4x4Matrix2));
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 Multiply(const XMMATRIX& xmmtxMatrix1, const XMMATRIX& xmmtx4x4Matrix2)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, xmmtxMatrix1 * xmmtx4x4Matrix2);
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 RotationYawPitchRoll(float fPitch, float fYaw, float fRoll)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(
		&xmmtx4x4Result,
		XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll))
	);
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 RotationAxis(const XMFLOAT3& xmf3Axis, float fAngle)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, XMMatrixRotationAxis(XMLoadFloat3(&xmf3Axis), XMConvertToRadians(fAngle)));
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 Inverse(const XMFLOAT4X4& xmmtx4x4Matrix)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, XMMatrixInverse(NULL, XMLoadFloat4x4(&xmmtx4x4Matrix)));
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 Transpose(const XMFLOAT4X4& xmmtx4x4Matrix)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(&xmmtx4x4Result, XMMatrixTranspose(XMLoadFloat4x4(&xmmtx4x4Matrix)));
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 PerspectiveFovLH(float fFovAngleY, float fAspectRatio, float fNearZ, float fFarZ)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(
		&xmmtx4x4Result, XMMatrixPerspectiveFovLH(XMConvertToRadians(fFovAngleY), fAspectRatio, fNearZ, fFarZ)
	);
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 LookAtLH(
	const XMFLOAT3& xmf3EyePosition, const XMFLOAT3& xmf3LookAtPosition, const XMFLOAT3& xmf3UpDirection
)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(
		&xmmtx4x4Result,
		XMMatrixLookAtLH(
			XMLoadFloat3(&xmf3EyePosition), XMLoadFloat3(&xmf3LookAtPosition), XMLoadFloat3(&xmf3UpDirection)
		)
	);
	return (xmmtx4x4Result);
}

inline XMFLOAT4X4 LookToLH(const XMFLOAT3& xmf3EyePosition, const XMFLOAT3& xmf3LookTo, const XMFLOAT3& xmf3UpDirection)
{
	XMFLOAT4X4 xmmtx4x4Result;
	XMStoreFloat4x4(
		&xmmtx4x4Result,
		XMMatrixLookToLH(XMLoadFloat3(&xmf3EyePosition), XMLoadFloat3(&xmf3LookTo), XMLoadFloat3(&xmf3UpDirection))
	);
	return (xmmtx4x4Result);
}
} // namespace Matrix4x4

namespace Triangle
{
using namespace DirectX;
inline bool Intersect(
	const XMFLOAT3& xmf3RayPosition,
	const XMFLOAT3& xmf3RayDirection,
	const XMFLOAT3& v0,
	const XMFLOAT3& v1,
	const XMFLOAT3& v2,
	float&			fHitDistance
)
{
	return (TriangleTests::Intersects(
		XMLoadFloat3(&xmf3RayPosition), XMLoadFloat3(&xmf3RayDirection), XMLoadFloat3(&v0), XMLoadFloat3(&v1),
		XMLoadFloat3(&v2), fHitDistance
	));
}
} // namespace Triangle

namespace Plane
{
using namespace DirectX;
inline XMFLOAT4 Normalize(const XMFLOAT4& xmf4Plane)
{
	XMFLOAT4 xmf4Result;
	XMStoreFloat4(&xmf4Result, XMPlaneNormalize(XMLoadFloat4(&xmf4Plane)));
	return (xmf4Result);
}
} // namespace Plane
