#pragma once

#include "Script.h"

namespace Server {
	namespace Contents {
		class Spawner : public Script {
		private:
			float					m_accDT;
			static constexpr auto	SOLDIER_SPAWN_TIME = 5s;
			static constexpr int	SPAWN_NPC_COUNT = 3;

		public:
			virtual void Update(const float dt) override;
		};
	}
}



