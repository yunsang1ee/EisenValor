#include "pch.h"
#include "LobbyServerGlobalFuncs.h"

#include "ClientSession.h"

std::shared_ptr<LobbyServerEngine::Session> MakeClientSessionFunc()
{
	return LobbyServerEngine::ObjectPool<LobbyServer::ClientSession>::MakeShared();
}