#pragma once

namespace ServerEngine {

	namespace RIO {
		class RIOSession;
		class RIOSessionTest;

#ifdef _USE_RIO
		class RIOContext : public RIO_BUF {
		protected:
			explicit RIOContext(IO_CONTEXT_TYPE type);
			friend class RIOSession;

		public:
			void Init();
			void SetOwner(std::shared_ptr<RIOSession> owner) { m_owner = owner; }
			std::shared_ptr<RIOSession> GetOwner() const { return m_owner; }

		public:
			IO_CONTEXT_TYPE GetType() const { return m_type; }

		private:
			IO_CONTEXT_TYPE								m_type;
			std::shared_ptr<RIOSession>					m_owner;

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
#endif
	}
}