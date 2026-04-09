#pragma once

#include "Script.h"

namespace GameServer {
	namespace Contents {
		class Spawner : public Script {
		public:
			virtual void Update(const float dt) override;

		private:
			float					m_accDT;
			static constexpr auto	SOLDIER_SPAWN_TIME = 15s;
			static constexpr int	SPAWN_NPC_COUNT = 1;
		};
	}
}