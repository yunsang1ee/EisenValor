#pragma once

#include "Creature.h"

namespace Server {
	namespace Contents{
		class Player;
			
		class NPC : public Creature {
		private:
			const NPC_TYPE			m_type;
			Vec3					m_targetPos;

		public:
			explicit NPC(const NPC_TYPE type, const TEAM_TYPE team);
			virtual ~NPC();

		public:
			const NPC_TYPE GetNpcType() const noexcept { return m_type; }

		public:
			virtual void Update(const float dt) override;

		public:
			void SetTargetPos(const Vec3& targetPos) noexcept { m_targetPos = targetPos; }
			const Vec3& GetTargetPos() const noexcept { return m_targetPos; }
		};
	}
}