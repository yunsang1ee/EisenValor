#pragma once
#include "Creature.h"

namespace Server {
	namespace Contents {
		class General : public Creature {
		protected:
			FB_ENUMS::GENERAL_STANCE_TYPE			m_stanceType;
			GENERAL_SUB_STATE_TYPE					m_subStateType;
			AttackInfo								m_atkInfo;
			uint64									m_startStunDelay;
			uint32									m_stunDelay;

			float									m_accDTForStaminaRecovery;
			float									m_accDTForRespawn;
			
		public:
			explicit General(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE objType = FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			virtual ~General();

		public:
			virtual void OnDeath() override;
			virtual void Respawn() override;
			bool IsTargetInAttackRange(GameObject* const target);

		public:
			void AddSubState(const GENERAL_SUB_STATE_TYPE subStateType) { m_subStateType |= subStateType; }
			void RemoveSubState(const GENERAL_SUB_STATE_TYPE subStateType) { m_subStateType &= ~subStateType; }
			bool HasSubState(const GENERAL_SUB_STATE_TYPE subStateType) const { return (m_subStateType & subStateType) != GENERAL_SUB_STATE_TYPE::NONE; }
			void ChangeSubState(const GENERAL_SUB_STATE_TYPE subStateType) { m_subStateType = subStateType; }
			
			void SetStanceType(const FB_ENUMS::GENERAL_STANCE_TYPE stanceType) noexcept { m_stanceType = stanceType; }
			void SetAtkInfo(const AttackInfo& atkInfo) { m_atkInfo = atkInfo; }
			FB_ENUMS::GENERAL_STANCE_TYPE GetStanceType() const noexcept { return m_stanceType; }
			const AttackInfo& GetAttackInfo() const noexcept { return m_atkInfo; }
	
		private:
		};
	}
}

