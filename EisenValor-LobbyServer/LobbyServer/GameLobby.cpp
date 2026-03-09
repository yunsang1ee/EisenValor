#include "pch.h"
#include "GameLobby.h"

#include "ClientSession.h"
#include "GameRoom.h"

LobbyServer::GameLobby::GameLobby()
{
}

LobbyServer::GameLobby::~GameLobby()
{
}

void LobbyServer::GameLobby::Broadcast(std::shared_ptr<LobbyServerEngine::PacketBuffer> pb)
{
	for(const auto& [id, session] : m_lobbySessions)
		session->Send(pb);
}