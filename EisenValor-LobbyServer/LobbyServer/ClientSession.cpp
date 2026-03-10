#include "pch.h"
#include "ClientSession.h"

#include "SessionManager.h"

LobbyServer::ClientSession::ClientSession()
	:Session{SESSION_TYPE::CLIENT}
{
}

LobbyServer::ClientSession::~ClientSession()
{
	std::cout << "~ClientSession" << std::endl;
}

void LobbyServer::ClientSession::OnConnected()
{
	LOG_INFO("Session ID:{}, OnConnected!", GetID());
	MANAGER(LobbyServer::SessionManager)->AddSession(std::static_pointer_cast<ClientSession>(shared_from_this()));
}

void LobbyServer::ClientSession::OnDisconnected(const std::string_view reason)
{
	LOG_INFO("Session ID:{}, OnDisconnected!", GetID());
	MANAGER(LobbyServer::SessionManager)->RemoveSession(std::static_pointer_cast<ClientSession>(shared_from_this()));
}

void LobbyServer::ClientSession::OnSend(const uint32 bytesTrasnferred)
{
}