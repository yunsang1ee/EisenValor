#pragma once
#include "BehaviorNode.h"

namespace GameServer {
	namespace Contents {
		class IsTargetInNearRange : public ConditionNode {
		public:
			// 근처에 적이 있는지 확인한다.
			virtual bool Check(const float dt) override final;
		};

		class IsInOccupationZone : public ConditionNode {
		public:
			// 점령지 안에 있는지 확인한다.
			virtual bool Check(const float dt) override final;
		};

		class FindOZ : public ActionNode {
		public:
			// 아직 점령되지 않은 점령지를 찾는다.
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};
	
		class MoveToOZ : public ActionNode {
		public:
			// 점령지를 향해 달려간다.
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		class IsRespawnReady : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
			void Reset() { m_accDTForRespawn = 0.f; }

		private:
			float m_accDTForRespawn{};
		};

		class ChangeState : public ActionNode {
			public:
			ChangeState(const FB_ENUMS::GENERAL_STATE_TYPE stateType) : m_stateType{ stateType } {}
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;

		private:
			FB_ENUMS::GENERAL_STATE_TYPE m_stateType;
		};

		class Respawn : public ActionNode {
			public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// ==================
		//	  DEFENSE_SEQ
		// ==================
		class IsTargetAttacking : public ConditionNode {
		public:
			// 상대방이 공격하고 있는 상태인지 확인한다
			virtual bool Check(const float dt) override final;
		};

		class DefaultDefense : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};
		 
		class Parrying : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// ==================
		//	  ATTACK_SEQ
		// ==================
		class AttackTry : public ActionNode {
		public:
			// 상대방을 공격한다.
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;

		private:
			float m_accDT=0.f;
		};

		class CombatMovement : public ActionNode {
		public:
			CombatMovement();

		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;

		private:
			float m_accDTForChangeAttackDir;
		};
	}
}


