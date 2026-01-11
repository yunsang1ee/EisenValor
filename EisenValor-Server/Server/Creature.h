#pragma once
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class Creature : public GameObject {
		private:
			bool						m_alive;
			CreatureStatInfo			m_statInfo;
			Creature*					m_target;

		public:
			Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type);
			virtual ~Creature();

		public:
			void	SetStatInfo(const CreatureStatInfo& stat) noexcept { m_statInfo = stat; }
			void	SetHp(const uint32 hp) noexcept;
			void	SetStamina(const int32 stamina) noexcept { m_statInfo.stamina = stamina; }
			void	SetAlive(const bool alive) noexcept { m_alive = alive; }
			void	SetTarget(Creature* target) { m_target = target; }
			Creature* GetTarget() { return m_target; }
			int		GetHP() const noexcept { return m_statInfo.hp; }
			int32	GetStamina() const noexcept { return m_statInfo.stamina; }

			void	IncHP(const uint32 amount);
			void	DecHP(const uint32 amount);
			void	IncStamina(const uint32 amount);
			void	DecStamina(const uint32 amount);

			bool	IsAlive() const noexcept { return m_alive; }

			const CreatureStatInfo& GetStatInfo() const noexcept { return m_statInfo; }

		public:
			// TODO: 이 함수를 Event에 넣어서 처리하면 되지 않을까?
			virtual bool OnDamaged(Creature* attacker, const float dt) { return true; }
		};
	}
}	