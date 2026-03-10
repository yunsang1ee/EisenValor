#include "pch.h"
#include "LobbyServerGlobalFuncs.h"

#include "ClientSession.h"
#include "GameServerSession.h"

std::shared_ptr<LobbyServerEngine::Session> MakeClientSessionFunc()
{
	return LobbyServerEngine::ObjectPool<LobbyServer::ClientSession>::MakeShared();
}

std::shared_ptr<LobbyServerEngine::Session> MakeGameServerSessionFunc()
{
	return LobbyServerEngine::ObjectPool<LobbyServer::GameServerSession>::MakeShared();
}