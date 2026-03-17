#include "pch.h"
#include "ServerGlobalFuncs.h"

#include "ClientSession.h"
#include "LobbyServerSession.h"
#include "GameWorld.h"
#include "GameObject.h"

std::shared_ptr<ServerEngine::Session> MakeClientSessionFunc()
{
	return ServerEngine::ObjectPool<ClientSession>::MakeShared();
}

std::shared_ptr<ServerEngine::Session> MakeLobbyServerSessionFunc()
{
	return ServerEngine::ObjectPool<LobbyServerSession>::MakeShared();
}

std::unique_ptr<ServerEngine::IRoom> MakeGameWorldFunc()
{
	return std::make_unique<Server::Contents::GameWorld>();
}

bool IsValidObj(const std::shared_ptr<Server::Contents::GameObject> obj)
{
	if(nullptr == obj)
		return false;

	if(false == obj->IsActive())
		return false;

	return true;
}