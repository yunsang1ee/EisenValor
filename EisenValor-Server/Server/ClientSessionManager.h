#pragma once

#include "Singleton.hpp"

namespace Server {
	class ClientSessionManager : public Singleton<ClientSessionManager> {
		SINGLETON(ClientSessionManager)
	
	private:
		tbb::concurrent_unordered_set<std::shared_ptr<ClientSession>> m_sessions;
	
	public:
		void AddSession(std::shared_ptr<ClientSession> clientSession);
		void RemoveSession(std::shared_ptr<ClientSession> clientSession);
		void Broadcast(std::shared_ptr<ClientSession> clientSession, std::shared_ptr<ServerEngine::PacketBuffer> buffer);
	};
}

