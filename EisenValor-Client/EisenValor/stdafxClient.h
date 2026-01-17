#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include <windows.h>

#include "Packets/Enums_generated.h"
#include "Packets/Structs_generated.h"
#include "Packets/Tables_generated.h"

#include "../EisenValor-ClientFramework/include/stdafxClientFramework.h"