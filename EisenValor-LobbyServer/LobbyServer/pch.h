#pragma once

#include "LobbyServerEnginePch.h"

#include "LobbyServerGlobalFuncs.h"

namespace LobbyServer {
	class GameLobby;
}

extern std::shared_ptr<LobbyServer::GameLobby> G_GAME_LOBBY;