#pragma once
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class Creature : public GameObject {
		private:
			Stat						m_statInfo;
			Creature*					m_target;

		public:
			explicit Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type);
			virtual ~Creature();

		public:
			virtual bool OnDamaged(Creature* const attacker, const float dt) { return false; }
			virtual void OnDeath() abstract;
			virtual void Respawn() {}
		
		public:
			void	SetStat(const Stat& stat) noexcept { m_statInfo = stat; }
			void	SetHp(const uint32 hp, const bool broadcast = false) noexcept;
			void	IncHP(const uint32 amount, const bool broadcast = true);
			void	DecHP(const uint32 amount, const bool broadcast = true);
			
			void	SetStamina(const uint32 stamina, const bool broadcast = false) noexcept;
			void	IncStamina(const uint32 amount, const bool broadcast = true);
			virtual void	DecStamina(const uint32 amount, const bool broadcast = true);
			void	SetTarget(Creature* target) { m_target = target; }
			Creature* GetTarget() { return m_target; }
			int		GetHP() const noexcept { return m_statInfo.currentHP; }
			int32	GetStamina() const noexcept { return m_statInfo.currentStamina; }

			void	IncRespawnTime();

			const Stat& GetStat() const noexcept { return m_statInfo; }

		private:
			void BroadcastUpdateVital();
		};
	}
}