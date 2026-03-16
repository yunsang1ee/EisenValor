#pragma once

#include "Singleton.hpp"

namespace LobbyServerEngine {
	class Session;
}

namespace LobbyServer {
	class GameServerSession;
	class ClientSession;

	class SessionManager : public Singleton<SessionManager> {
		SINGLETON(SessionManager)

	public:
		void AddSession(std::shared_ptr<LobbyServerEngine::Session> session);
		void RemoveSession(std::shared_ptr<LobbyServerEngine::Session> session);
		void Broadcast(std::shared_ptr<LobbyServerEngine::Session> session, std::shared_ptr<LobbyServerEngine::PacketBuffer> buffer);
	
		std::shared_ptr<GameServerSession> GetGameServerSession() const { return m_gameServerSession; }
		
	private:
		tbb::concurrent_unordered_set<std::shared_ptr<ClientSession>>	m_clientSessions;
		std::shared_ptr<GameServerSession>								m_gameServerSession;
	};
}


