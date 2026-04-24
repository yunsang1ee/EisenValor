#pragma once

#include "Script.h"

namespace GameServer {
	namespace Contents {
		class OccupationZone : public Script {
		public:
			explicit OccupationZone(const float rangeSq, const int64 scoreTime);
			virtual ~OccupationZone() = default;

		public:
			virtual void Update(const float dt) override final;

		public:
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE GetStateType() const { return m_stateType; }
			float GetRangeSq() const { return m_rangeSq; }

		private:
			FB_ENUMS::TEAM_TYPE GetDominantTeamType();
			void CheckOccupationState(const float prev, const float curr);
			void BroadcastGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType);

		private:
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE	m_stateType;
			FB_ENUMS::TEAM_TYPE						m_prevDominantTeamType;

			float									m_rangeSq;
			std::chrono::seconds					m_scoreTime;
			float									m_gauge;
			float									m_rateOfGaugeIncrease;

			float									m_lastSentGauge;
			float									m_syncAccDT;
			float									m_dominantAccDT;
		};
	}
}


