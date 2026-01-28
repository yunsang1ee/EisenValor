#pragma once

namespace ServerEngine {

	namespace IOCP {
		class IOCPSession;

		class IOCPContext : public OVERLAPPED {
		private:
			IO_CONTEXT_TYPE					m_type;
			std::shared_ptr<IOCPSession>	m_owner;

		public:
			explicit IOCPContext(const IO_CONTEXT_TYPE type);

		public:
			void Init();
		
		public:
			void SetOwner(std::shared_ptr<IOCPSession> owner) { m_owner = owner; }
			std::shared_ptr<IOCPSession> GetOwner() const { return m_owner; }
			IO_CONTEXT_TYPE GetType() const { return m_type; }

		};

		class IOCPAcceptContext : public IOCPContext {
		private:
			// Session을 미리 연결해두어도 됨
			SOCKET m_acceptSocket;

		public:
			IOCPAcceptContext() : IOCPContext{ IO_CONTEXT_TYPE::ACCEPT }, m_acceptSocket{ INVALID_SOCKET } {}

		public:
			void SetAcceptSocket(const SOCKET acceptSocket) { m_acceptSocket = acceptSocket; }
			SOCKET GetAcceptSocket() const { return m_acceptSocket; }
		};

		class IOCPRecvContext : public IOCPContext {
		public:
			IOCPRecvContext() :IOCPContext{ IO_CONTEXT_TYPE::RECV } {}

		};

		class IOCPSendContext : public IOCPContext {
		public:
			IOCPSendContext() : IOCPContext(IO_CONTEXT_TYPE::SEND) {}
		};
	}
}


