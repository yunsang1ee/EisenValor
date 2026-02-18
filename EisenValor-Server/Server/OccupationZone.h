#pragma once

#include "Script.h"

namespace Server {
	namespace Contents {
		class OccupationZone : public Script {
		private:
			float									m_rangeSq;
			std::chrono::milliseconds				m_time;
			float									m_accDT;
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE	m_stateType;

		public:
			explicit OccupationZone(const float rangeSq, const int64 time);
			virtual ~OccupationZone() = default;

		public:
			virtual void Update(const float dt) override final;

		public:
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE GetStateType() const { return m_stateType; }
		};
	}
}


