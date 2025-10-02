#pragma once
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class Creature : public GameObject {
		private:
			StatInfo					m_stat;
			bool						m_alive;
			std::weak_ptr<Creature>		m_target;

		public:
			Creature(const GAME_OBJECT_TYPE type, const TEAM_TYPE teamType);
			virtual ~Creature();

		public:
			void	SetStatInfo(const StatInfo& stat) noexcept { m_stat = stat; }
			void	SetHp(const int hp) noexcept { m_stat.hp = hp; }
			void	SetAtk(const int32 atk) noexcept { m_stat.atk = atk; }
			void	SetStamina(const int32 stamina) noexcept { m_stat.stamina = stamina; }
			void	SetAlive(const bool alive) noexcept { m_alive = alive; }
			void	SetTarget(std::shared_ptr<Creature> target) { m_target = target; }
			std::shared_ptr<Creature> GetTarget() { return m_target.lock(); }
			int		GetHP() const noexcept { return m_stat.hp; }
			int32	GetAtk() const noexcept { return m_stat.atk; }
			int32	GetStamina() const noexcept { return m_stat.stamina; }
			bool	IsAlive() const noexcept { return m_alive; }
		};
	}
}	