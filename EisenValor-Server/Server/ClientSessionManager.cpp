#include "pch.h"
#include "ClientSessionManager.h"

#include "ClientSession.h"

void Server::ClientSessionManager::AddSession(std::shared_ptr<ClientSession>&& clientSession)
{
	std::lock_guard<tbb::spin_mutex> lk{ m_mutex };
	if(false == m_sessions.contains(clientSession))
		m_sessions.insert(std::move(clientSession));
}

void Server::ClientSessionManager::RemoveSession(std::shared_ptr<ClientSession>&& clientSession)
{
	std::lock_guard<tbb::spin_mutex > lk{ m_mutex };
	if(m_sessions.contains(clientSession))
		m_sessions.erase(std::move(clientSession));
}

void Server::ClientSessionManager::Broadcast(std::shared_ptr<ClientSession> clientSession, std::shared_ptr<ServerEngine::PacketBuffer> buffer)
{
	 std::lock_guard<tbb::spin_mutex > lk{ m_mutex };
	for(const auto& session : m_sessions) {
		if( session != clientSession && SESSION_STATE::ALLOC == session->GetState()) {
			session->Send(buffer);
		}
	}
}