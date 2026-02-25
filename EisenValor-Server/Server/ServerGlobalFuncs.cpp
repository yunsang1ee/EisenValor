#include "pch.h"
#include "ServerGlobalFuncs.h"

#include "ClientSession.h"
#include "GameObject.h"

std::shared_ptr<Server::Contents::GameLobby> G_GAME_LOBBY;

std::shared_ptr<ClientSession> MakeClientSessionFunc()
{
	return ServerEngine::ObjectPool<ClientSession>::MakeShared();
}

bool IsValidObj(const Server::Contents::GameObject* const obj)
{
	if(nullptr == obj || false == obj->IsActive())
		return false;

	return true;
}
