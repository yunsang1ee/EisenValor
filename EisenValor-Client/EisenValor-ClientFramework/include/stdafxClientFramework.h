// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#pragma region Preprocessor Macro
#define WIN32_LEAN_AND_MEAN		// 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _WIN32_WINNT 0x0A00
#define NOMINMAX

#pragma endregion

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

// C++ Containers
#include <unordered_map>
#include <vector>
#include <array>

// My
#include "DxCommon.h"
#include "DxMath.h"
#include <Global.h>

#pragma endregion

#pragma region Library
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma endregion


#pragma region NetworkLibrary
#include "NetBridgePch.h"

#ifdef _DEBUG
#pragma comment(lib, "NetBridge\\Debug\\NetBridge_Debug.lib")
#else
#pragma comment(lib, "NetBridge\\Release\\NetBridge_Release.lib")
#endif
#pragma endregion


#pragma region DebugHelpers
std::string GetTimestamp();

#ifdef _DEBUG
#define DEBUG_LOG_FMT(fmt, ...)                                                      \
    do {                                                                             \
        std::cout << GetTimestamp();                                                 \
        std::cout << std::format((fmt), __VA_ARGS__);                                \
    } while (false)
#else
#define DEBUG_LOG_FMT(fmt, ...) ((void)0)
#endif

#pragma endregion

#pragma region Variable
constexpr size_t kFrameBufferWidth = 1920;
constexpr size_t kFrameBufferHeight = 1080;

constexpr size_t kExplosionDebrises = 240;

#pragma endregion