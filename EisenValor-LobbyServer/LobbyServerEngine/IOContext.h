#pragma once

namespace LobbyServerEngine {
	class Session;
	class IOCPObject;

	class IOContext : public OVERLAPPED {
	public:
		explicit IOContext(const IO_CONTEXT_TYPE type);

	public:
		void Init();

	public:
		void SetOwner(std::shared_ptr<IOCPObject> owner) { m_owner = owner; }
		std::shared_ptr<IOCPObject>GetOwner() const { return m_owner; }
		IO_CONTEXT_TYPE GetType() const { return m_type; }

	private:
		IO_CONTEXT_TYPE					m_type;
		std::shared_ptr<IOCPObject>		m_owner;
	};

	class ConnectContext final : public IOContext {
	public:
		ConnectContext() : IOContext{IO_CONTEXT_TYPE::CONNECT} { }
	};

	class AcceptContext final : public IOContext {
	public:
		AcceptContext() : IOContext{ IO_CONTEXT_TYPE::ACCEPT }, m_session{ nullptr } {}

	public:
		void SetSession(std::shared_ptr<Session> session) { m_session = session; }
		std::shared_ptr<Session> GetSession() const { return m_session; }

	private:
		std::shared_ptr<Session> m_session;
	};

	class RecvContext final : public IOContext {
	public:
		RecvContext() :IOContext{ IO_CONTEXT_TYPE::RECV } {}
	};

	class SendContext final : public IOContext {
	public:
		std::vector<std::shared_ptr<PacketBuffer>> m_packetBuffers;
		SendContext() : IOContext(IO_CONTEXT_TYPE::SEND) {}
	};
}

