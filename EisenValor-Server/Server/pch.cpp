#include "pch.h"

#include "ClientSession.h"

std::shared_ptr<Server::Contents::GameLobby> G_GAME_LOBBY;

std::shared_ptr<ClientSession> MakeClientSessionFunc()
{
	return ServerEngine::ObjectPool<ClientSession>::MakeShared();
}
