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
			Player(const FB_ENUMS::TEAM_TYPE teamType);
			virtual ~Player();

		public:
			void SetSession(std::shared_ptr<ClientSession> clientSession) noexcept { m_session = clientSession; }
		
		public:
			std::shared_ptr<ClientSession> GetSession() { return m_session.lock(); }
		
		public:
			virtual bool OnDamaged(std::shared_ptr<Creature> attacker, const int32 damaged, const float dt) override;

		};
	}
}


