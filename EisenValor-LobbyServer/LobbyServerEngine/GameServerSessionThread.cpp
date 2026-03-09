#include "pch.h"
#include "GameServerSessionThread.h"

LobbyServerEngine::GameServerSessionThread::GameServerSessionThread()
	:m_gameServerSocket{INVALID_SOCKET}
{
}

LobbyServerEngine::GameServerSessionThread::~GameServerSessionThread()
{
}

bool LobbyServerEngine::GameServerSessionThread::Init()
{
	return true;
}