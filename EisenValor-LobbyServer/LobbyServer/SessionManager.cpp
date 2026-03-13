#include "pch.h"
#include "SessionManager.h"

#include "ClientSession.h"
#include "GameServerSession.h"

void LobbyServer::SessionManager::AddSession(std::shared_ptr<LobbyServerEngine::Session>  session)
{
	const SESSION_TYPE sessionType{ session->GetType() };

	switch(sessionType) {
		case SESSION_TYPE::CLIENT:
		{
			auto clientSession{ std::static_pointer_cast<ClientSession>(session) };
			
			if(m_clientSessions.contains(clientSession))
				m_clientSessions.insert(std::move(clientSession));
			break;
		}
		case SESSION_TYPE::GAME_SERVER:
		{
			if(m_gameServerSession)
				return;

			m_gameServerSession = std::static_pointer_cast<GameServerSession>(session);
			break;
		}
		default:
			break;
	}
}

void LobbyServer::SessionManager::RemoveSession(std::shared_ptr<LobbyServerEngine::Session>  session)
{
	const SESSION_TYPE sessionType{ session->GetType() };

	switch(sessionType) {
		case SESSION_TYPE::CLIENT:
		{
			auto clientSession{ std::static_pointer_cast<ClientSession>(session) };

			if(m_clientSessions.contains(clientSession))
				m_clientSessions.unsafe_erase(clientSession);
			break;
		}
		case SESSION_TYPE::GAME_SERVER:
		{
			m_gameServerSession = nullptr;
			break;
		}
		default:
			break;
	}
}

void LobbyServer::SessionManager::Broadcast(std::shared_ptr<LobbyServerEngine::Session>  session, std::shared_ptr<LobbyServerEngine::PacketBuffer> buffer)
{
	for(const auto& clientSession : m_clientSessions) {
		if(clientSession != session && SESSION_STATE::FREE != clientSession->GetState()) {
			clientSession->Send(buffer);
		}
	}
}