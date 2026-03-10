#include "pch.h"
#include "GameServerSession.h"

LobbyServer::GameServerSession::GameServerSession()
	:Session{ SESSION_TYPE::GAME_SERVER }
{
}

LobbyServer::GameServerSession::~GameServerSession()
{
}

void LobbyServer::GameServerSession::OnConnected()
{
	std::cout << "GameServerSession OnConnected!" << std::endl;
}

void LobbyServer::GameServerSession::OnDisconnected(const std::string_view reason)
{
}

void LobbyServer::GameServerSession::OnSend(const uint32 bytesTransferred)
{
}