#pragma once

#include "LobbyServerEnginePch.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "LobbyServerEnums.h"
#include "LobbyServerGlobalFuncs.h"
#include "LobbyServerStructs.h"

namespace LobbyServer {
	class GameLobby;
}

extern std::shared_ptr<LobbyServer::GameLobby> G_GAME_LOBBY;

#include "SendPackets.h"
