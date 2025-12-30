#pragma once

#define DEG2RAD 180.f / 3.141592f
#define DEVELOP

#pragma warning(disable: 4819)
#ifdef _DEBUG
#pragma comment(lib, "ServerEngine\\Debug\\ServerEngine_Debug.lib")
#else
#pragma comment(lib, "ServerEngine\\Release\\ServerEngine_Release.lib")
#endif

#include "ServerEnginePch.h"
#include "flatbuffers\\flatbuffers.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"
#include "ServerTypes.h"
#include "ServerEnums.h"
#include "ServerStructs.h"
#include "ServerGlobalFunc.h"

#include "ClientPacketHandler.h"
#include "ServerPackets.h"
#include "GameObjectFactory.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class GameLobby;
	}
}


std::shared_ptr<Server::ClientSession> MakeClientSessionFunc();

extern std::shared_ptr<Server::Contents::GameLobby> G_GAME_LOBBY;