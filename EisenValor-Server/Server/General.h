#pragma once
#include "Creature.h"

namespace GameServer {
	namespace Contents {
		class General : public Creature {
		public:
			explicit General(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE objType = FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			virtual ~General();	

		public:
			virtual void Update(const float dt) override;
			virtual void OnDeath() override;
			virtual void OnRespawn() override;
			virtual bool OnDamaged(std::shared_ptr<Creature> const attacker, const float dt, const bool broadcast = true) override;
			bool IsTargetInAttackRange(std::shared_ptr<GameObject> const target);
			virtual void OnPostComponentUpdate(const float dt) override final;
		public:
			void AddSubState(const GENERAL_SUB_STATE_TYPE subStateType) { m_subStateType |= subStateType; }
			void RemoveSubState(const GENERAL_SUB_STATE_TYPE subStateType) { m_subStateType &= ~subStateType; }
			bool HasSubState(const GENERAL_SUB_STATE_TYPE subStateType) const { return (m_subStateType & subStateType) != GENERAL_SUB_STATE_TYPE::NONE; }
			void ChangeSubState(const GENERAL_SUB_STATE_TYPE subStateType) { m_subStateType = subStateType; }
			GENERAL_SUB_STATE_TYPE GetSubState() const { return m_subStateType; }
			
			void SetStanceType(const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
			void SetAtkInfo(const AttackInfo& atkInfo) { m_atkInfo = atkInfo; }
			void SetAtkDir(const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType) { m_atkInfo.dir = dirType; }
			void SetMoveDir(const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir) { m_moveDir = moveDir; }

			FB_ENUMS::MOVE_DIRECTION_TYPE GetMoveDir() const { return m_moveDir; }
			FB_ENUMS::GENERAL_STANCE_TYPE GetStanceType() const { return m_stanceType; }
			const AttackInfo& GetAtkInfo() const { return m_atkInfo; }

			void SetRespawnPos(const Vec3& pos) { m_respawnPos = pos; }	

		protected:
			FB_ENUMS::MOVE_DIRECTION_TYPE			m_moveDir;
			FB_ENUMS::GENERAL_STANCE_TYPE			m_stanceType;
			GENERAL_SUB_STATE_TYPE					m_subStateType;
			AttackInfo								m_atkInfo;
			uint64									m_startStunDelay;
			uint32									m_stunDelay;

			float									m_accDTForStaminaRecovery;
			float									m_accDTForRespawn;

			Vec3									m_respawnPos;

		};
	}
}

