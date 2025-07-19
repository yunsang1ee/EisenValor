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
		std::set<std::shared_ptr<Session>>				m_connectedSession;
		std::mutex										m_mutex;

	public:
		explicit RIOWorker(const uint16 id);
		~RIOWorker();

	public:
		bool			Init(SessionFactoryFunc sessionFunc);
		void			FlushPacketQueue();
		void			DequeueCompletion();
		void			ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr);
		void			ReleaseSession(std::shared_ptr<Session> session);
	public:
		const RIO_CQ&	GetCQ() const noexcept { return m_cq; }
		SessionPool*	GetSessionPool() noexcept { return m_sessionPool.get(); }
	};
}

