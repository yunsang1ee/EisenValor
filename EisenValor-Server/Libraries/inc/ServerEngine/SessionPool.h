#pragma once

namespace ServerEngine {
	class Session;

	class SessionPool {
	private:
		std::queue<std::shared_ptr<Session>>			m_freeSessions;
		std::unordered_set<std::shared_ptr<Session>>	m_allocedSessions;

	public:
		void Init(SessionFactoryFunc sessionFunc);
		void EnqSession(std::shared_ptr<Session> session);
		std::shared_ptr<Session> DeqSession();

	};
}


