#include "pch.h"
#include "ClientSessionManager.h"

#include "ClientSession.h"

void Server::ClientSessionManager::AddSession(std::shared_ptr<ClientSession> clientSession)
{
	if(false == m_sessions.contains(clientSession))
		m_sessions.insert(std::move(clientSession));
}

void Server::ClientSessionManager::RemoveSession(std::shared_ptr<ClientSession> clientSession)
{
	if(m_sessions.contains(clientSession))
		m_sessions.unsafe_erase(clientSession);
}

void Server::ClientSessionManager::Broadcast(std::shared_ptr<ClientSession> clientSession, std::shared_ptr<ServerEngine::PacketBuffer> buffer)
{
	for(const auto& session : m_sessions) {
		if(session != clientSession && SESSION_STATE::FREE != session->GetState()) {
			session->Send(buffer);
		}
	}
}