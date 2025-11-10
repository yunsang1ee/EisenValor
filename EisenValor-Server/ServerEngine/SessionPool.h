#pragma once

namespace ServerEngine {
	class Session;
	// acceptThread, RioWorker°¡ Á¢±Ù
	class SessionPool {
	private:
		SessionFactoryFunc										m_func;
		tbb::concurrent_queue<std::shared_ptr<Session>>			m_freeSessions;
		// std::mutex										m_sessionQueueMutex;
		// std::queue<std::shared_ptr<Session>>			m_freeSessions;
	
	public:
		void Init(SessionFactoryFunc sessionFunc);
		void EnqSession(std::shared_ptr<Session> session);
		std::shared_ptr<Session> DeqSession();

	};
}