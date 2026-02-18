#pragma once

#include "SessionPool.h"

namespace ServerEngine {
	class RIOCore;
	class SessionPool;

	namespace RIO {
		class RIOSession;

#ifdef _USE_RIO
		class RIOWorker {
		private:
			RIO_CQ															m_cq;
			uint16															m_id;
			tbb::concurrent_unordered_set<std::shared_ptr<RIOSession>>		m_connectedSession;
			SessionPool														m_sessionPool;
			std::vector<RIORESULT>											m_ioResults;
			SessionFactoryFunc												m_sessionFactoryFunc;

		public:
			explicit RIOWorker(const uint16 id);
			~RIOWorker();

		public:
			bool			Init(SessionFactoryFunc sessionFunc);
			void			Work();
			bool			ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr);

		public:
			RIO_CQ			GetCQ() const { return m_cq; }
			uint16			GetID() const { return m_id; }
			auto& GetSessionPool() { return m_sessionPool; }

		private:
			// 관리하고 있는 Session들의 각각 보낼 Packet들 처리
			void			FlushSessionPacketQueue();
			void			DequeueCompletion();
		};

#endif
	}

}

