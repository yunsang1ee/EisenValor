#pragma once

namespace ServerEngine {
	class Session;

	enum class RIO_CONTEXT_TYPE {
		RECV,
		SEND,
	};

	class RIOContext : public RIO_BUF {
	private:
		RIO_CONTEXT_TYPE			m_type;
		std::shared_ptr<Session>	m_session;

	public:
		explicit RIOContext(RIO_CONTEXT_TYPE type);

	public:
		void Init();
		void HoldSession(std::shared_ptr<Session> session) { m_session = session; }
		void ReleaseSession() { m_session = nullptr; }
		std::shared_ptr<Session> GetSession() const noexcept { return m_session; }

	public:
		RIO_CONTEXT_TYPE GetType() const noexcept { return m_type; }

	};

	class RecvContext : public RIOContext {
	public:
		RecvContext();
	};

	// TODO: ObjectPool을 만들어서 SendContext 재사용 해야함.
	class SendContext : public RIOContext {
	public:
		SendContext();
	};
}