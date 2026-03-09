#pragma once
#pragma warning(disable: 4819)

#include "LobbyServerEngineDefines.h"

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

// tbb
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_set.h>
#include <tbb/tbb.h>
#include <tbb/memory_pool.h>
#include <tbb/scalable_allocator.h>

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
#include <variant>

using namespace std::chrono;
namespace fs = std::filesystem;

#include "LobbyServerEngineTypes.h"
#include "LobbyServerEngineEnums.h"

#include "ObjectPool.h"

#include "LobbyServerTLS.h"
#include "LogManager.h"

#include "PacketHeader.h"
#include "PacketBuffer.h"

namespace LobbyServerEngine {
	class Session;
}

using SessionFactoryFunc = std::function<std::shared_ptr<LobbyServerEngine::Session>()>;
