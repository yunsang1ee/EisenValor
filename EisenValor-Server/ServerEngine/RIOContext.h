#pragma once

namespace ServerEngine {
	class RIOSession;

	class RIOContext : public RIO_BUF {
	private:
		IO_CONTEXT_TYPE							m_type;
		std::atomic<std::shared_ptr<Session>>	m_session;

	protected:
		explicit RIOContext(IO_CONTEXT_TYPE type);
		friend class RIOSession;

	public:
		void Init();
		void HoldSession(std::shared_ptr<Session> session) { m_session.store(session); }
		void ReleaseSession() {  m_session.exchange(nullptr); }
		std::shared_ptr<Session> GetSession() const noexcept { return m_session.load(); }

	public:
		IO_CONTEXT_TYPE GetType() const noexcept { return m_type; }

	};

	class RecvContext : public RIOContext {
	private:
		RecvContext() : RIOContext{ IO_CONTEXT_TYPE::RECV } {};
		friend class RIOSession;
	};

	class SendContext : public RIOContext {
	public:
		SendContext() : RIOContext{ IO_CONTEXT_TYPE::SEND } {};
	};

}