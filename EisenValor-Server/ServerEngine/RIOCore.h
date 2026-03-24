#pragma once

#include "IOCore.h"

namespace GameServerEngine {
	namespace RIO {
		class RIOSession;
		class RIOCore : public IOCore {
		public:
			RIOCore();
			virtual ~RIOCore();

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
			RIO_EXTENSION_FUNCTION_TABLE									m_rioExtfuncTable;
			std::unordered_set<std::shared_ptr<RIOSession>>					m_connectedSessions;

		};
	}
}