#pragma once

#include "SessionPool.h"

namespace ServerEngine {
	class RIOCore;
	class SessionPool;
	class Session;

	class RIOWorker {
	private:
		RIO_CQ											m_cq;
		uint16											m_id;

		// TODO: 이거 vector로 들고있을 이유가 딱히 없음, Set이 적당함
		std::vector<std::shared_ptr<Session>>			m_connectedSession;
		
		tbb::spin_mutex									m_mutex;

		// acceptThread가 접근
		SessionPool										m_sessionPool;

	public:
		explicit RIOWorker(const uint16 id);
		~RIOWorker();

	public:
		bool			Init(SessionFactoryFunc sessionFunc) noexcept;
		void			Work() noexcept;

		void			ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr) noexcept;

	public:
		const RIO_CQ&	GetCQ() const noexcept { return m_cq; }
		uint16	GetID() const noexcept { return m_id; }
		auto& GetSessionPool() noexcept { return m_sessionPool; }

	private:
		// 관리하고 있는 Session들의 각각 보낼 Packet들 처리
		void			FlushSessionPacketQueue() noexcept;
		void			DequeueCompletion() const noexcept;
	};
}

