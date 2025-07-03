#include "pch.h"
#include "ClientSessionManager.h"

#include "ClientSession.h"

void Server::ClientSessionManager::AddSession(std::shared_ptr<ClientSession> clientSession)
{
	std::lock_guard<std::mutex> lk{ m_mutex };
	if(false == m_sessions.contains(clientSession))
		m_sessions.insert(clientSession);
}

void Server::ClientSessionManager::RemoveSession(std::shared_ptr<ClientSession> clientSession)
{
	std::lock_guard<std::mutex> lk{ m_mutex };
	if(m_sessions.contains(clientSession))
		m_sessions.erase(clientSession);
}

void Server::ClientSessionManager::Broadcast()
{
	std::lock_guard<std::mutex> lk{ m_mutex };
	for(const auto& session : m_sessions) {
		// TODO: Send
		// session->Send();
	}
}
