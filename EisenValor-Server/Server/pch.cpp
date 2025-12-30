#include "pch.h"

#include "ClientSession.h"

std::shared_ptr<Server::ClientSession> MakeClientSessionFunc()
{
	return ServerEngine::ObjectPool<Server::ClientSession>::MakeShared();
}

std::shared_ptr<Server::Contents::GameLobby> G_GAME_LOBBY;