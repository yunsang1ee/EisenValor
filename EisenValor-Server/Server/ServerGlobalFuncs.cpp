#include "pch.h"
#include "ServerGlobalFuncs.h"

#include "ClientSession.h"
#include "LobbyServerSession.h"
#include "GameWorld.h"
#include "GameObject.h"

std::shared_ptr<GameServerEngine::Session> MakeClientSessionFunc()
{
	return GameServerEngine::ObjectPool<ClientSession>::MakeShared();
}

std::shared_ptr<GameServerEngine::Session> MakeLobbyServerSessionFunc()
{
	return GameServerEngine::ObjectPool<LobbyServerSession>::MakeShared();
}

std::unique_ptr<GameServerEngine::IRoom> MakeGameWorldFunc()
{
	return std::make_unique<GameServer::Contents::GameWorld>();
}

bool IsValidObj(const std::shared_ptr<GameServer::Contents::GameObject> obj)
{
	if(nullptr == obj)
		return false;

	if(false == obj->IsActive())
		return false;

	return true;
}