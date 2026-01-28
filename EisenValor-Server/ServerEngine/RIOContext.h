#pragma once

namespace ServerEngine {

	namespace RIO {
		class RIOSession;
		
		class RIOContext : public RIO_BUF {
		private:
			IO_CONTEXT_TYPE								m_type;
			std::atomic<std::shared_ptr<RIOSession>>	m_session;

		protected:
			explicit RIOContext(IO_CONTEXT_TYPE type);
			friend class RIOSession;

		public:
			void Init();
			void HoldSession(std::shared_ptr<RIO::RIOSession> session) { m_session.store(session); }
			void ReleaseSession() { m_session.exchange(nullptr); }
			std::shared_ptr<RIOSession> GetSession() const noexcept { return m_session.load(); }

		public:
			IO_CONTEXT_TYPE GetType() const noexcept { return m_type; }

		};

		class RIORecvContext : public RIOContext {
		private:
			RIORecvContext() : RIOContext{ IO_CONTEXT_TYPE::RECV } {};
			friend class RIOSession;
		};

		class RIOSendContext : public RIOContext {
		public:
			RIOSendContext() : RIOContext{ IO_CONTEXT_TYPE::SEND } {};
		};
	}

}