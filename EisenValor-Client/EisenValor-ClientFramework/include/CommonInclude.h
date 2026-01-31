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

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

namespace Epsilon
{
constexpr float kMachineEpsilon = 1.192092896e-7f;
constexpr float kEpsilon4 = 1e-4f;
} // namespace Epsilon

namespace Variable
{
constexpr uint32 kInvalidServerID = 0;

constexpr size_t kDefaultWindowWidth = 1920;
constexpr size_t kDefaultWindowHeight = 1080;
} // namespace Variable

#pragma region NetworkLibrary
#include "flatbuffers\\flatbuffers.h"

#include "NetworkGlobal.h"
#include "IPacketHandler.h"
#pragma endregion