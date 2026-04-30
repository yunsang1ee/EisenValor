#pragma once

#include "Creature.h"

namespace GameServer {
	namespace Contents {
		class Soldier : public Creature {
		public:
			explicit Soldier(const FB_ENUMS::TEAM_TYPE teamType);
			virtual ~Soldier();

		public:
			virtual void Update(const float dt) override final;
			virtual void OnDeath() override final;
			virtual bool OnDamaged(std::shared_ptr<Creature> const attacker, const float dt, const bool broadcast = true) override final;
			virtual void OnPostComponentUpdate(const float dt) override final;
		private:
			friend class GameWorld;
			friend class GameObjectFactory;
		};
	}
}