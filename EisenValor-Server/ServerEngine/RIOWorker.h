#pragma once

namespace ServerEngine {
	class RIOCore;
	class SessionPool;
	class Session;

	class RIOWorker {
	private:
		RIO_CQ											m_cq;
		std::unique_ptr<SessionPool>					m_sessionPool;
		int32											m_id;
		std::vector<std::shared_ptr<Session>>			m_connectedSession;
		
		tbb::spin_mutex									m_mutex;

	public:
		explicit RIOWorker(const uint16 id);
		~RIOWorker();

	public:
		bool			Init(SessionFactoryFunc sessionFunc) noexcept;
		void			Work() noexcept;

		void			ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr) noexcept;

	public:
		const RIO_CQ&	GetCQ() const noexcept { return m_cq; }
		SessionPool*	GetSessionPool() noexcept { return m_sessionPool.get(); }

	private:
		void			FlushSessionPacketQueue() noexcept;
		void			DequeueCompletion() const noexcept;
	};
}

