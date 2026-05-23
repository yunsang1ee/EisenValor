#pragma once
#pragma warning(disable: 4819)

// #define APPLY_LOBBY_SERVER

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

#include "flatbuffers/flatbuffers.h"

// headers
#include "ServerEngineTypes.h"
#include "ServerEngineGlobalVariables.h"
#include "ServerEngineEnums.h"
#include "ServerEngineStruct.h"
#include "ServerEngineTLS.h"

#include "LogManager.h"
#include "ThreadManager.h"

#include "PacketHeader.h"
#include "PacketBuffer.h"

#include "Timer.h"

#include "ServerEngineContainers.h"
#include "ObjectPool.h"
#include "LockQueue.h"

// inc
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "JobQueue.h"

std::string		WStringToString(const std::wstring_view wstr);
std::wstring	StringToWString(const std::string_view str);
SOCKET			CreateSocket(const DWORD flags);

namespace GameServer {
	class RIOClientSession;
	class IOCPClientSession;

	class RIOLobbyServerSession;
	class IOCPLobbyServerSession;

	class GameWorld;
}

#ifdef _USE_RIO
using ClientSession = GameServer::RIOClientSession;
using LobbyServerSession = GameServer::RIOLobbyServerSession;
#endif

#ifdef _USE_IOCP
using ClientSession = Server::IOCPClientSession;
using LobbyServerSession = Server::IOCPLobbyServerSession;
#endif
