#pragma once

#include "GameObject.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class Soldier;
		class General : public GameObject {
		private:
			std::weak_ptr<ClientSession>			m_session;
			std::vector<std::shared_ptr<Soldier>>	m_soldiers;

		public:
			General();
			virtual ~General() = default;

		public:
			void SetSession(std::weak_ptr<ClientSession> clientSession) noexcept { m_session = clientSession; }

		public:
			void AddSoldier(std::shared_ptr<Soldier> soldier);

		public:
			std::shared_ptr<ClientSession> GetOwner() { return m_session.lock(); }
			const auto& GetSoldiers() const noexcept { return m_soldiers; }
		};
	}
}


