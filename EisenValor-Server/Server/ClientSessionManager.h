#pragma once

#include "Singleton.hpp"

namespace Server {
	class ClientSession;

	class ClientSessionManager : public Singleton<ClientSessionManager> {
		SINGLETON(ClientSessionManager)
	
	private:
		// TODO: TBB·Î ¼öÁ¤
		std::mutex m_mutex;
		std::unordered_set<std::shared_ptr<ClientSession>> m_sessions;
	
	public:
		void AddSession(std::shared_ptr<ClientSession> clientSession);
		void RemoveSession(std::shared_ptr<ClientSession> clientSession);
		
		// TODO: Broadcast
		void Broadcast();
	};
}

