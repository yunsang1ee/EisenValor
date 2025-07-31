#pragma once

#include "ServerEngineDefines.h"

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
#include <print>
#include <bitset>

// STL
#include <array>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <unordered_set>
#include <queue>

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

// functinal
#include <limits>
#include <filesystem>
#include <source_location>
#include <functional>
#include <cassert>

using namespace std::chrono;
namespace fs = std::filesystem;

// headers
#include "ServerEngineTypes.h"
#include "ServerEngineGlobalVariables.h"
#include "ServerEngineEnums.h"
#include "ServerEngineTLS.h"

#include "LogManager.h"
#include "ThreadManager.h"

#include "PacketHeader.h"
#include "PacketBuffer.h"

#include "Timer.h"

#include "ServerEngineContainers.h"
#include "ObjectPool.h"
#include "LockQueue.h"
