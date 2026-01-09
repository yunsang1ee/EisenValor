// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#pragma region Preprocessor Macro
#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _WIN32_WINNT 0x0A00
#define NOMINMAX

#define SINGLETON_CLASS(classname)                                                                                     \
private:                                                                                                               \
	classname() = default;                                                                                             \
	~classname() = default;                                                                                            \
	classname(const classname&) = delete;                                                                              \
	classname& operator=(const classname&) = delete;                                                                   \
	classname(classname&&) = delete;                                                                                   \
	classname& operator=(classname&&) = delete;                                                                        \
                                                                                                                       \
public:                                                                                                                \
	static classname* GetInstance()                                                                                    \
	{                                                                                                                  \
		static classname instance;                                                                                     \
		return &instance;                                                                                              \
	}


#pragma endregion

#define GLOBAL(classname) (classname::GetInstance())
// Types
using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

// Enums
enum
{
	NW_BUFFER_CAPACITY = 65536,
};

// Structs


enum class SOLDIER_FORMATION
{
	FORMATION_1,
	FORMATION_2,
	FORMATION_3,

	END
};


#pragma region Header
// Windows 헤더 파일:
#include <windows.h>

// C의 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cmath>

// C++
#include <iostream>
#include <format>
#include <concepts>
#include <ranges>
#include <memory>
#include <random>
#include <print>
#include <numeric>
#include <functional>
#include <chrono>

// C++ Containers
#include <unordered_map>
#include <vector>
#include <array>

// Network
#include <WS2tcpip.h>
#include <MSWSock.h>

// Container
#include "DenseList.h"
template <typename T>
using HandleOf = DenseListHandle<T>;

// DirectX
#include "DxCommon.h"
// #include "DxMath.h"

namespace epsilon
{
constexpr float kMachineEpsilon = 1.192092896e-7f;
} // namespace epsilon

#include "SimpleMath.h"
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;
using Quaternion = DirectX::SimpleMath::Quaternion;
using Ray = DirectX::SimpleMath::Ray;

#pragma endregion

#pragma region Library
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#pragma endregion


#pragma region NetworkLibrary
#include "flatbuffers\\flatbuffers.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "NetworkGlobal.h"
#include "IPacketHandler.h"
#pragma endregion

#pragma region DebugHelpers
std::string GetTimestamp();

#ifdef _DEBUG
#define DEBUG_LOG_FMT(fmt, ...)                                                                                        \
	do                                                                                                                 \
	{                                                                                                                  \
		std::cout << GetTimestamp();                                                                                   \
		std::cout << std::format((fmt), __VA_ARGS__);                                                                  \
	} while (false)
#else
#define DEBUG_LOG_FMT(fmt, ...) ((void)0)
#endif

#pragma endregion

#pragma region Utils
namespace Utils
{
// FNV-1a Hash https://share.google/trFAqACv1zHhll7h8
constexpr uint64_t HashString(std::string_view str)
{
	uint64_t hash = 14695981039346656037ULL;
	for (char c : str)
	{
		hash ^= static_cast<uint8_t>(c);
		hash *= 1099511628211ULL;
	}
	return hash;
}
} // namespace Utils
#pragma endregion

#pragma region Variable
constexpr uint32 kInvalidServerID = 0;

constexpr size_t kDefaultWindowWidth = 1920;
constexpr size_t kDefaultWindowHeight = 1080;

#pragma endregion

// 오브젝트 타입 구분용
enum class ObjectType
{
	PLAYER,
	NPC,
	SPAWN_BASE,


	END
};


// #define DEAD_RECKONING