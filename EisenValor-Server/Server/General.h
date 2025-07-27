#pragma once

#include "GameObject.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class General : public GameObject {
		private:
			std::weak_ptr<ClientSession> m_session;

		public:
			General();
			virtual ~General() = default;

		public:
			void SetSession(std::weak_ptr<ClientSession> clientSession) noexcept { m_session = clientSession; }

		public:
			std::shared_ptr<ClientSession> GetOwner() { return m_session.lock(); }
		};
	}
}


