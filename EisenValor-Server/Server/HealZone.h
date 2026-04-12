#pragma once

#include "Script.h"

namespace GameServer {
	namespace Contents {
		class HealZone : public Script {
		public:
			explicit HealZone(const float rangeSq, const int64 time, const uint32 healAmount);
			virtual ~HealZone() = default;

		public:
			virtual void Update(const float dt) override final;

		private:
			const float									m_rangeSq;
			const std::chrono::seconds					m_time;
			const uint32								m_healAmount;
			std::unordered_map<uint64, float> 			m_healObjects;
		};
	}
}
