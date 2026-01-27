#pragma once

#define NOMINMAX

#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include <conio.h>

// Util
#include <chrono>
#include <iostream>
#include <fstream>
#include <format>
#include <bitset>

// STL
#include <array>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <unordered_set>
#include <span>

// multi-thread
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>
#include <shared_mutex>
#include <syncstream>

// functinal
#include <limits>
#include <filesystem>
#include <source_location>
#include <functional>
#include <cassert>
#include <random>

#include "../../EisenValor-Server/ServerEngine/ServerEngineTypes.h"
#include "../../EisenValor-Server/ServerEngine/PacketHeader.h"

#include "NetBridgeDefines.h"
#include "NetBridgeEnums.h"

#include "flatbuffers\\flatbuffers.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "NetworkManager.h"
#include "PacketHandler.h"

