#pragma once

#include "IOCore.h"
#include "RIOWorker.h"

namespace ServerEngine {
	namespace RIO {
		class RIOWorker;

#ifdef _USE_RIO
#ifdef LEGACY_CODE
		class RIOCore : public IOCore {
		public:
			RIOCore();
			virtual ~RIOCore() = default;

			RIOCore(const RIOCore&) = delete;
			RIOCore(RIOCore&&) = default;
			RIOCore& operator=(const RIOCore&) = delete;
			RIOCore& operator=(RIOCore&&) = default;

		public:
			[[nodiscard("DO NOT IGNORE RETURN VALUE")]]
			virtual bool	Init(const SessionFactoryFunc sessionFunc) override final;
			[[nodiscard("DO NOT IGNORE RETURN VALUE")]]
			virtual bool	StartAccept() override final;
			virtual void	Run() override final;

		public:
			const auto&		GetRioExtFuncTB() const { return m_rioExtfuncTable; }
			virtual void	Shutdown() override final;

		private:
			void			DoAcceptLoop();

		private:
			RIO_EXTENSION_FUNCTION_TABLE			m_rioExtfuncTable;
			uint16									m_acceptThreadNum;
			std::vector<std::unique_ptr<RIOWorker>>	m_rioWorkers;
		};
#endif
#endif
	}
}