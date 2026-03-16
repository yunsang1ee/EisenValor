#include "pch.h"
#include "SessionManager.h"

#include "ClientSession.h"
#include "LobbyServerSession.h"

void Server::SessionManager::AddSession(std::shared_ptr<ServerEngine::Session> session)
{
	const SESSION_TYPE sessionType{ session->GetType() };

	switch(sessionType) {
		case SESSION_TYPE::CLIENT:
		{
			auto clientSession{ std::static_pointer_cast<ClientSession>(session) };

			if(m_clientSessions.contains(clientSession))
				return;

			m_clientSessions.insert(std::move(clientSession));
			break;
		}
		case SESSION_TYPE::LOBBY_SERVER:
		{
			if(m_lobbyServerSession)
				return;

			m_lobbyServerSession = std::static_pointer_cast<LobbyServerSession>(session);
			break;
		}
		default:
			break;
	}
}

void Server::SessionManager::RemoveSession(std::shared_ptr<ServerEngine::Session> session)
{
	const SESSION_TYPE sessionType{ session->GetType() };

	switch(sessionType) {
		case SESSION_TYPE::CLIENT:
		{
			auto clientSession{ std::static_pointer_cast<ClientSession>(session) };

			if(false == m_clientSessions.contains(clientSession))
				return;

			m_clientSessions.unsafe_erase(clientSession);
			break;
		}
		case SESSION_TYPE::LOBBY_SERVER:
		{
			m_lobbyServerSession = nullptr;
			break;
		}
		default:
			break;
	}
}

void Server::SessionManager::Broadcast(std::shared_ptr<ServerEngine::Session> session, std::shared_ptr<ServerEngine::PacketBuffer> buffer)
{
	for(const auto& clientSession : m_clientSessions) {
		if(session != clientSession && SESSION_STATE::FREE != session->GetState()) {
			session->Send(buffer);
		}
	}
}