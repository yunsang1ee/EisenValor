#pragma once
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class Creature : public GameObject {
		private:
			StatInfo	m_stat;
			bool		m_alive;

		public:
			Creature(const GAME_OBJECT_TYPE type, const TEAM_TYPE team);
			virtual ~Creature();

		public:
			void	SetStatInfo(const StatInfo& stat) noexcept { m_stat = stat; }
			void	SetHp(const int hp) noexcept { m_stat.hp = hp; }
			int		GetHP() const noexcept { return m_stat.hp; }
			void	SetAlive(const bool alive) noexcept { m_alive = alive; }
			bool	IsAlive() const noexcept { return m_alive; }
		};
	}
}