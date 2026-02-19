#pragma once

#include "Creature.h"

namespace Server {
	namespace Contents {
		class BattleRam : public Creature {
		public:
			explicit BattleRam(const float detecionRange, const Vec3& finalDestPos);
			virtual ~BattleRam();

		public:
			virtual void OnDeath() override final;
			virtual void Update(const float dt) override final;

		private:
			const float		m_detectionRangeSq;
			const Vec3		m_finalDestPos;
		};
	}
}


