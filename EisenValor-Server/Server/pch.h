#pragma once
// define ENABLE_LOBBY

#pragma warning(disable: 4819)

#include "ServerEnginePch.h"
#include "flatbuffers\\flatbuffers.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"
#include "ServerTypes.h"
#include "ServerEnums.h"
#include "ServerStructs.h"
#include "ServerGlobalFunc.h"
#include "GameDataManager.h"
#include "ClientPacketHandler.h"
#include "ServerPackets.h"
#include "GameObjectFactory.h"

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "DetourCrowd.h"

namespace ServerEngine {
	class Session;
}

namespace Server {
	namespace Contents {
		class GameLobby;
	}

	class RIOClientSession;
	class IOCPClientSession;
}

extern std::shared_ptr<Server::Contents::GameLobby> G_GAME_LOBBY;

std::shared_ptr<ClientSession> MakeClientSessionFunc();

extern inline std::mt19937_64 mersenne{ std::random_device{}() };