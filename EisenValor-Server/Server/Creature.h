#pragma once
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class Creature : public GameObject {
		private:
			bool						m_alive;
			CreatureStat				m_statInfo;
			Creature*					m_target;

		public:
			explicit Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type);
			virtual ~Creature();

		public:
			virtual bool OnDamaged(Creature* const attacker, const float dt) { return false; }
			virtual void OnDeath() abstract;
			virtual void Respawn() {}
		
		public:
			void	SetStat(const CreatureStat& stat) noexcept { m_statInfo = stat; }
			void	SetHp(const uint32 hp) noexcept;
			void	SetStamina(const int32 stamina) noexcept { m_statInfo.currentStamina = stamina; }
			void	SetAlive(const bool alive) noexcept { m_alive = alive; }
			void	SetTarget(Creature* target) { m_target = target; }
			Creature* GetTarget() { return m_target; }
			int		GetHP() const noexcept { return m_statInfo.currentHP; }
			int32	GetStamina() const noexcept { return m_statInfo.currentStamina; }

			void	IncHP(const uint32 amount);
			void	DecHP(const uint32 amount);
			void	IncStamina(const uint32 amount);
			virtual void	DecStamina(const uint32 amount);
			void	IncRespawnTime();

			bool	IsAlive() const noexcept { return m_alive; }

			const CreatureStat& GetStat() const noexcept { return m_statInfo; }
		};
	}
}