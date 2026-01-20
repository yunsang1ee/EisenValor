#pragma once

#include "SessionPool.h"

namespace ServerEngine {
	class RIOCore;
	class SessionPool;
	class Session;

	class RIOWorker {
	private:
		RIO_CQ														m_cq;
		uint16														m_id;

		tbb::concurrent_unordered_set<std::shared_ptr<Session>>		m_connectedSession;

		SessionPool													m_sessionPool;
		std::vector<RIORESULT>										m_ioResults;
	
	public:
		explicit RIOWorker(const uint16 id);
		~RIOWorker();

	public:
		bool			Init(SessionFactoryFunc sessionFunc) noexcept;
		void			Work() noexcept;
		bool			ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr) noexcept;

	public:
		RIO_CQ			GetCQ() const noexcept { return m_cq; }
		uint16			GetID() const noexcept { return m_id; }
		auto&			GetSessionPool() noexcept { return m_sessionPool; }

	private:
		// 관리하고 있는 Session들의 각각 보낼 Packet들 처리
		void			FlushSessionPacketQueue() noexcept;
		void			DequeueCompletion() noexcept;
	};
}

