#pragma once

#include "Script.h"

namespace GameServer {
	namespace Contents {
		class Spawner : public Script {
		public:
			explicit Spawner(const Vec3& destPos, const uint32 spawnTimeSec, const uint32 spawnCount);
			virtual ~Spawner() = default;

		public:
			virtual void Update(const float dt) override;

		private:
			const Vec3					m_soldierDestPos;
			std::chrono::seconds	m_spawnTimeSec;
			const uint32				m_spawnCount;

			float						m_accDT;
		};
	}
}