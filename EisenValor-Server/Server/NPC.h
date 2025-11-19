#pragma once

#include "Creature.h"

namespace Server {
	namespace Contents{
		class Player;
			
		class NPC : public Creature {
		private:
			const FB_ENUMS::NPC_TYPE			m_type;
			Vec3								m_targetPos;

		public:
			explicit NPC(const FB_ENUMS::NPC_TYPE type, const FB_ENUMS::TEAM_TYPE team);
			virtual ~NPC();

		public:
			const FB_ENUMS::NPC_TYPE GetNpcType() const noexcept { return m_type; }

		public:
			virtual void Update(const float dt) override;

		public:
			void SetTargetPos(const Vec3& targetPos) noexcept { m_targetPos = targetPos; }
			const Vec3& GetTargetPos() const noexcept { return m_targetPos; }
			
		public:
			virtual bool OnDamaged(std::shared_ptr<Creature> attacker, const int32 damaged, const float dt) override;
		};
	}
}