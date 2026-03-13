#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class Session;
}

namespace Server {
	class SessionManager : public Singleton<SessionManager> {
		SINGLETON(SessionManager)
	public:
		void AddSession(std::shared_ptr<ServerEngine::Session> session);
		void RemoveSession(std::shared_ptr<ServerEngine::Session> session);
		void Broadcast(std::shared_ptr<ServerEngine::Session> session, std::shared_ptr<ServerEngine::PacketBuffer> buffer);

	private:
		tbb::concurrent_unordered_set<std::shared_ptr<ClientSession>>		m_clientSessions;
		std::shared_ptr<LobbyServerSession>									m_lobbyServerSession;

	};
}

