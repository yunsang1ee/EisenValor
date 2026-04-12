#pragma once
#include "GameObject.h"

namespace GameServer {
	namespace Contents {
		class Creature : public GameObject {
		public:
			explicit Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type);
			virtual ~Creature();

		public:
			virtual bool OnDamaged(std::shared_ptr<Creature> const attacker, const float dt, const bool broadcast = true) { return false; }
			virtual void OnDeath() {}
			virtual void OnRespawn() {}
		
		public:
			void	SetStat(const Stat& stat) { m_statInfo = stat; }
			void	SetHp(const uint32 hp, const bool broadcast = false);
			void	IncHP(const uint32 amount, const bool broadcast = true);
			void	DecHP(const uint32 amount, const bool broadcast = true);
			
			void	SetStamina(const uint32 stamina, const bool broadcast = false);
			void	IncStamina(const uint32 amount, const bool broadcast = true);
			virtual void	DecStamina(const uint32 amount, const bool broadcast = true);
			void	SetTarget(std::shared_ptr<Creature> target) { m_target = target; }
			std::shared_ptr<Creature> GetTarget() { return m_target.lock(); }
			int		GetHP() const { return m_statInfo.currentHP; }
			int32	GetStamina() const { return m_statInfo.currentStamina; }

			void	IncRespawnTime();

			const Stat& GetStat() const { return m_statInfo; }

			void BroadcastUpdateVital();
	
		private:
			// TODO: Component로 뺴는것도 생각해보자
			Stat						m_statInfo;
			std::weak_ptr<Creature>		m_target;

		};
	}
}