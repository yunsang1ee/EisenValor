#pragma once

#include "Creature.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class NPC;

		class Player : public Creature {
		private:
			std::weak_ptr<ClientSession>			m_session;

		public:
			Player();
			virtual ~Player();

		public:
			void SetSession(std::weak_ptr<ClientSession> clientSession) noexcept { m_session = clientSession; }
		
		public:
			std::shared_ptr<ClientSession> GetOwner() { return m_session.lock(); }

		public:
			virtual void Update(const float dt) override;
		
		};
	}
}


