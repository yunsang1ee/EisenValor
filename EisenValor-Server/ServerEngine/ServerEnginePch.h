#pragma once

#include "ServerEngineDefines.h"

#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include <conio.h>

#include <chrono>
#include <iostream>
#include <fstream>
#include <format>
#include <print>
#include <bitset>

#include <array>
#include <vector>
#include <queue>
#include <unordered_set>

#include <thread>
#include <memory>
#include <mutex>
#include <atomic>

#include <filesystem>
#include <source_location>
#include <functional>
#include <cassert>

#include "ServerEngineTypes.h"
#include "ServerEngineGlobalVariables.h"
#include "ServerEngineEnums.h"
#include "ServerEngineTLS.h"

#include "LogManager.h"
#include "ThreadManager.h"

using namespace std::chrono;
namespace fs = std::filesystem;

#pragma pack(push, 1)
struct PacketHeader {
	uint16		packetType;
	uint16		packetSize;	// PacketHeader 크기 포함
};
#pragma pack(pop)