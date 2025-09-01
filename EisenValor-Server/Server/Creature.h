#pragma once
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class Creature : public GameObject {
		public:
			Creature(const GAME_OBJECT_TYPE type, const TEAM_TYPE team);
			virtual ~Creature() = default;
		};
	}
}