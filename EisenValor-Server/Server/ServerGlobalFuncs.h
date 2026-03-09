#pragma once

namespace ServerEngine {
	class IRoom;
}

namespace Server {
	class RIOClientSession;
	class IOCPClientSession;
}

std::shared_ptr<ClientSession> MakeClientSessionFunc();
#ifdef MODERN_CODE
std::unique_ptr<ServerEngine::IRoom> MakeGameWorldTest();
std::unique_ptr<ServerEngine::IRoom> MakeGameLobbyTest();
#endif

static inline FB_STRUCTS::Vec3 Vec3ToFlatVec3(const Vec3& v)
{
	return FB_STRUCTS::Vec3{ v.x, v.y, v.z };
}

static inline Vec3 FlatVec3ToVec3(const FB_STRUCTS::Vec3& v)
{
	return Vec3{ v.x(), v.y(), v.z() };
}

static inline Vec3 FlatVec3ToVec3(const FB_STRUCTS::Vec3* v)
{
	return Vec3{ v->x(), v->y(), v->z() };
}

static inline float Deg2Rad(const float degree)
{
	return degree * (DirectX::XM_PI / 180.f);
}

static inline float Rad2Deg(const float radian)
{
	return radian * (180.f / DirectX::XM_PI);
}

static inline float NormalizeAngle(float angle)
{
	angle = fmodf(angle + 180.0f, 360.0f);
	if(angle < 0) angle += 360.0f;
	return angle - 180.0f;
}

static inline bool TryLuck(const double probability)
{
	static std::bernoulli_distribution d{ probability };
	return d(mersenne);
}

static inline float GetDistSq(const Vec3& v1, const Vec3& v2)
{
	return (v1 - v2).LengthSquared();
}

bool IsValidObj(const std::shared_ptr<Server::Contents::GameObject> obj);

template<typename Enum> requires std::is_enum_v<Enum>
static inline constexpr uint8 etou8(const Enum e)
{
	return static_cast<uint8>(e);
}

inline constexpr GENERAL_SUB_STATE_TYPE operator|(const GENERAL_SUB_STATE_TYPE a, const GENERAL_SUB_STATE_TYPE b)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator|=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b)
{
	a = a | b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator&(const GENERAL_SUB_STATE_TYPE a, const GENERAL_SUB_STATE_TYPE b)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator&=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b)
{
	a = a & b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator^(const GENERAL_SUB_STATE_TYPE a, const GENERAL_SUB_STATE_TYPE b)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator^=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b)
{
	a = a ^ b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator~(const GENERAL_SUB_STATE_TYPE a)
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(~static_cast<uint8>(a));
}