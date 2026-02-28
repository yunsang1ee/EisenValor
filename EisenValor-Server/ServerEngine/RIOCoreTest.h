#pragma once

#include "IOCPCoreTest.h"

namespace ServerEngine {
	namespace RIO {
		class RIOSession;
#ifdef MODERN_CODE
		// WorkerThread마다 소유
		class RIOCoreTest : public IOCoreTest {
		public:
			RIOCoreTest();
			virtual ~RIOCoreTest();

		public:
			virtual bool Init() override final;
			virtual bool Register(std::shared_ptr<Session> session) override final;
			virtual bool Deregister(std::shared_ptr<Session> session) override final;

			virtual void ProcessIO() override final;

		public:
			const auto& GetRioExtFuncTable() const { return m_rioExtfuncTable; }

		private:
			void FlushPacketQueue();
			void DequeueCompletion();

		private:
			RIO_CQ															m_cq;
			std::vector<RIORESULT>											m_ioResults;
			std::unordered_map<uint32, std::shared_ptr<RIOSession>>			m_connectedSessions;
			RIO_EXTENSION_FUNCTION_TABLE									m_rioExtfuncTable;

		};
#endif
	}
}