#pragma once

#include "Creature.h"

namespace Server {
	namespace Contents {
		class Soldier : public Creature {
		private:

		public:
			Soldier(const FB_ENUMS::TEAM_TYPE teamType);
			virtual ~Soldier();
			
		};
	}
}


