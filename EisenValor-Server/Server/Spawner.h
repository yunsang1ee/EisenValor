#pragma once

#include "Script.h"

namespace Server {
	namespace Contents {
		class Spawner : public Script {
		private:
			float m_accDT;
			static constexpr auto SOLDIER_SPAWN_TIME = 10s;

		public:
			virtual void Update(const float dt) override;
		};
	}
}



