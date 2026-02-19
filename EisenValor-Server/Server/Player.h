#pragma once

#include "General.h"

namespace Server {
	namespace Contents {
		class NPC;

		class Player : public General {
		private:
			std::weak_ptr<ClientSession>			m_session;
			
		public:
			explicit Player(const FB_ENUMS::TEAM_TYPE teamType);
			virtual ~Player();
		
		public:
			virtual void Update(const float dt) override final;
			virtual bool OnDamaged(Creature* const attacker, const float dt) override final;
			virtual void OnDeath() override final;
			virtual void OnRespawn() override final;
			virtual void DecStamina(const uint32 amount, const bool broadcast= false) override;

		public:
			void SetSession(std::shared_ptr<ClientSession> clientSession) { m_session = clientSession; }
			std::shared_ptr<ClientSession> GetSession() { return m_session.lock(); }

		private:
			void Handle_CS_PLAYER_ATTACK(const FB_STRUCTS::GeneralAttackInfo& atkInfo);
			void Handle_CS_PLAYER_CHANGE_STANCE();
			void Handle_CS_PLAYER_FAKE();
			void Handle_CS_CHANGE_CAMERA_TARGET(const uint32 prevTargetID);
			friend class GameWorld;
			friend class GameObjectFactory;
		};
	}
}


