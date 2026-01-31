#pragma once

#include "IOCore.h"
#include "IOCPContext.h"

namespace ServerEngine {
	namespace IOCP {
#ifdef _USE_IOCP
		class IOCPCore : public ServerEngine::IOCore {
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
			void Work(const uint32 timeoutMs);
			void ProcessAccept(const IOCPAcceptContext* const acceptContext);

		private:
			static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
			static void Close(SOCKET& socket);

			void RegistAccept();

			std::shared_ptr<IOCPSession> CreateSession();

		};
#endif
	}
}



