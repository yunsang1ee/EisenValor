#pragma once

#include "Script.h"

namespace GameServer {
	namespace Contents {
		class OccupationZone : public Script {
		public:
			explicit OccupationZone(const float rangeSq, const int64 time);
			virtual ~OccupationZone() = default;

		public:
			virtual void Update(const float dt) override final;

		public:
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE GetStateType() const { return m_stateType; }
			float GetRangeSq() const { return m_rangeSq; }

		private:
			float									m_rangeSq;
			std::chrono::milliseconds				m_time;
			float									m_accDT;
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE	m_stateType;

		};
	}
}


