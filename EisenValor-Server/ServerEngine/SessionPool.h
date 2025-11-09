#pragma once

namespace ServerEngine {
	class Session;

	class SessionPool {
	private:
		SessionFactoryFunc								m_func;

		// TODO: LockFree-Queue·Î ¼öÁ¤
		std::mutex										m_sessionQueueMutex;
		std::queue<std::shared_ptr<Session>>			m_freeSessions;

	public:
		void Init(SessionFactoryFunc sessionFunc);
		void EnqSession(std::shared_ptr<Session> session);
		std::shared_ptr<Session> DeqSession();

	};
}