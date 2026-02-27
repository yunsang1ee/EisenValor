#pragma once

#include "IOCPCoreTest.h"

namespace ServerEngine {
	namespace RIO {
		class RIOSession;

		class RIOCoreTest : public IOCoreTest {
		public:
			RIOCoreTest() = default;
			virtual ~RIOCoreTest() = default;

		public:
			virtual bool Init() override final;
			virtual bool Register(std::shared_ptr<Session> session) override final;
			virtual void ProcessIO() override final;

		private:
			RIO_CQ											m_cq;
			std::vector<RIORESULT>							m_ioResults;
			std::unordered_set<std::shared_ptr<RIOSession>> m_connectedSessions;
			RIO_EXTENSION_FUNCTION_TABLE					m_rioExtfuncTable;

		};
	}
}