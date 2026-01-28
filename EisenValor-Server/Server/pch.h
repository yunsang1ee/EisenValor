#pragma once

#define DEG2RAD 180.f / 3.141592f
// define ENABLE_LOBBY

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

#include "AttackDataTable.h"
#include "StatDataTable.h"

#include "ClientPacketHandler.h"
#include "ServerPackets.h"
#include "GameObjectFactory.h"

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "DetourCrowd.h"


struct AttackInfo {
	AttackData* atkData;
	FB_ENUMS::GENERAL_ATTACK_DIR_TYPE	dir;
	uint64								startPreDelay;
	uint64								startPostDelay;
};



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