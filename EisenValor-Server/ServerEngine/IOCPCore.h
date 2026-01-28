#pragma once

#include "IOCore.h"
#include "IOCPContext.h"

namespace ServerEngine {
	namespace IOCP {
		class IOCPCore : public ServerEngine::IOCore {
		public:
			static LPFN_CONNECTEX					ConnectEx;
			static LPFN_DISCONNECTEX				DisconnectEx;
			static LPFN_ACCEPTEX					AcceptEx;

		private:
			HANDLE									m_iocpHandle;
			IOCPAcceptContext						m_acceptContext;

		public:
			IOCPCore();
			virtual ~IOCPCore() = default;

		public:
			virtual bool Init(const SessionFactoryFunc func) override final;
			virtual bool StartAccept() override final;
			virtual void Run() override final;
			virtual void Shutdown() override final;

		public:
			bool RegistHandle(const SOCKET socket);
			void Dispatch(const uint32 timeoutMs);
			void ProcessAccept(IOCPAcceptContext* acceptContext);

		private:
			static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
			static void Close(SOCKET& socket);

			void RegistAccept();

			std::shared_ptr<IOCPSession> CreateSession();

		};
	}
}



