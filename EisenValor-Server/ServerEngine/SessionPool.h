#pragma once

namespace ServerEngine {
	class Session;
#ifdef LEGACY_CODE
	class SessionPool {
	public:
		void Init(SessionFactoryFunc sessionFunc);
		std::shared_ptr<Session> DeqSession();
	
	private:
		SessionFactoryFunc										m_func;
		// acceptThread(DeqSession), RioWorker(EnqSession)가 접근하여서 concurrent_queue로 만들었음
		tbb::concurrent_queue<std::shared_ptr<Session>>			m_freeSessions;

	};
#endif
}