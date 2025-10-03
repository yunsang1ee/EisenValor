#pragma once

#include "Creature.h"

namespace Server {
	namespace Contents{
		class Player;
			
		class NPC : public Creature {
		private:
			const NPC_TYPE			m_type;
			Vec3					m_targetPos;
			std::weak_ptr<Player>	m_target;

		public:
			explicit NPC(const NPC_TYPE type, const TEAM_TYPE team);
			virtual ~NPC();

		public:
			const NPC_TYPE GetNpcType() const noexcept { return m_type; }

		public:
			void SetTarget(std::weak_ptr<Player> target) noexcept { m_target = target; }
			std::shared_ptr<Player> GetTarget() noexcept { return m_target.lock(); }
			void SetTargetPos(const Vec3& targetPos) noexcept { m_targetPos = targetPos; }
			const Vec3& GetTargetPos() const noexcept { return m_targetPos; }
		};
	}
}