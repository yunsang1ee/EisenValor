#pragma once

namespace LobbyServerEngine {
	class Session;

	class IOContext : public OVERLAPPED {
	public:
		explicit IOContext(const IO_CONTEXT_TYPE type);

	public:
		void Init();

	public:
		void SetOwner(std::shared_ptr<Session> owner) { m_owner = owner; }
		std::shared_ptr<Session> GetOwner() const { return m_owner; }
		IO_CONTEXT_TYPE GetType() const { return m_type; }

	private:
		IO_CONTEXT_TYPE					m_type;
		std::shared_ptr<Session>		m_owner;
	};

	class AcceptContext : public IOContext {
	public:
		AcceptContext() : IOContext{ IO_CONTEXT_TYPE::ACCEPT }, m_acceptSocket{ INVALID_SOCKET } {}

	public:
		void SetAcceptSocket(const SOCKET acceptSocket) { m_acceptSocket = acceptSocket; }
		SOCKET GetAcceptSocket() const { return m_acceptSocket; }
		char* GetBuff() { return buff; }

	private:
		// Session을 미리 연결해두어도 됨
		SOCKET m_acceptSocket;
		char buff[1024]{};
	};

	class RecvContext : public IOContext {
	public:
		RecvContext() :IOContext{ IO_CONTEXT_TYPE::RECV } {}
	};

	class SendContext : public IOContext {
	public:
		std::vector<std::shared_ptr<PacketBuffer>> m_packetBuffers;
		SendContext() : IOContext(IO_CONTEXT_TYPE::SEND) {}
	};
}

