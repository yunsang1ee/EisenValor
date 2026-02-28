#include "pch.h"
#include "ServerGlobalFuncs.h"

#include "ClientSession.h"
#include "GameLobby.h"
#include "GameWorld.h"
#include "GameObject.h"

std::shared_ptr<Server::Contents::GameLobby> G_GAME_LOBBY;

std::shared_ptr<ClientSession> MakeClientSessionFunc()
{
	return ServerEngine::ObjectPool<ClientSession>::MakeShared();
}

#ifdef MODERN_CODE
std::unique_ptr<ServerEngine::IRoom> MakeGameWorldTest()
{
	return std::make_unique<Server::Contents::GameWorldTest>();
}

std::unique_ptr<ServerEngine::IRoom> MakeGameLobbyTest()
{
	return std::make_unique<Server::Contents::GameLobbyTest>();
}
#endif

bool IsValidObj(const Server::Contents::GameObject* const obj)
{
	if(nullptr == obj || false == obj->IsActive())
		return false;

	return true;
}
