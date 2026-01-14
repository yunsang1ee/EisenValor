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
			virtual void OnDeath() override final;
		};
	}
}


