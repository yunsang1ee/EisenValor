#pragma once

#include "Creature.h"

namespace Server {
	namespace Contents {
		class Soldier : public Creature {
		private:

		public:
			explicit Soldier(const FB_ENUMS::TEAM_TYPE teamType);
			virtual ~Soldier();

		public:
			virtual void OnCollisionEnter(Collider* const other) override final;
			virtual void OnCollisionStay(Collider* const other) override final;
			virtual void OnCollisionExit(Collider* const other) override final;
			virtual void Update(const float dt) override final;
			virtual void OnDeath() override final;

		};
	}
}