#pragma once

namespace ServerEngine {
	class RIOCore;
	class SessionPool;
	class Session;

	class RIOWorker : public std::enable_shared_from_this<RIOWorker> {
	private:
		RIO_CQ											m_cq;
		std::unique_ptr<SessionPool>					m_sessionPool;
		int												m_id;

	public:
		explicit RIOWorker(const uint16 id);
		~RIOWorker();

	public:
		bool Init(SessionFactoryFunc sessionFunc);
		void WorkIO();
		void ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr);

	public:
		RIO_CQ& GetCQ() noexcept { return m_cq; }
		SessionPool* GetSessionPool() noexcept { return m_sessionPool.get(); }
	};
}

