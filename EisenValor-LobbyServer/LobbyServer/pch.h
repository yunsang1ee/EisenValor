#pragma once

#define size32(val)		static_cast<int32>(sizeof(val))

#ifndef ASSERT_CRASH
#define ASSERT_CRASH(expr) assert(expr)
#endif

#include "LobbyServerEnginePch.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "LobbyServerEnums.h"
#include "LobbyServerGlobalFuncs.h"
#include "LobbyServerStructs.h"
#include "PacketEnums.h"

namespace LobbyServer {
	class GameLobby;
}

extern std::shared_ptr<LobbyServer::GameLobby> G_GAME_LOBBY;

#include "SendPackets.h"

#include <sql.h>
#include <sqlext.h>