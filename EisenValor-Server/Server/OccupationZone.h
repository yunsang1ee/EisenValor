#pragma once

#include "Script.h"

namespace GameServer {
	namespace Contents {
		class OccupationZone : public Script {
		public:
			explicit OccupationZone(const float rangeSq, const int64 scoreTime, const float recaptureGraceSec);
			virtual ~OccupationZone() = default;

		public:
			virtual void Update(const float dt) override final;

		public:
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE GetStateType() const { return m_stateType; }
			FB_ENUMS::TEAM_TYPE GetOwnerTeamType() const { return m_ownerTeamType; }
			float GetRangeSq() const { return m_rangeSq; }

		public:
			bool IsInOccupationZone(const Vec3& pos) const;

		private:
			FB_ENUMS::TEAM_TYPE GetDominantTeamType() const;
			void UpdateOwnerScore(const float dt);
			bool UpdateGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType, const float dt);
			bool CanMoveGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType, const float dt);
			void MoveGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType, const float dt);
			void CheckOccupationState(const float prev, const float curr);
			void OnNeutralized();
			void OnOccupied(const FB_ENUMS::TEAM_TYPE teamType);
			void BroadcastGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType);

		private:
			FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE	m_stateType;
			FB_ENUMS::TEAM_TYPE						m_ownerTeamType;
			FB_ENUMS::TEAM_TYPE						m_prevDominantTeamType;

			float									m_rangeSq;
			std::chrono::seconds					m_scoreTime;
			float									m_recaptureGraceSec;
			float									m_gauge;
			float									m_rateOfGaugeIncrease;

			float									m_lastSentGauge;
			float									m_syncAccDT;
			float									m_scoreAccDT;
			float									m_graceAccDT;
		};
	}
}


