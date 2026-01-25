#pragma once

namespace ServerEngine {
	class Session;
	class SessionPool {
	private:
		SessionFactoryFunc										m_func;
		// acceptThread(DeqSession), RioWorker(EnqSession)가 접근하여서 concurrent_queue로 만들었음
		tbb::concurrent_queue<std::shared_ptr<Session>>			m_freeSessions;
	
	public:
		void Init(SessionFactoryFunc sessionFunc);
		std::shared_ptr<Session> DeqSession();

	};
}