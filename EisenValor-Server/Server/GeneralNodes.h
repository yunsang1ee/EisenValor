#pragma once
#include "BehaviorNode.h"

namespace GameServer {
	namespace Contents {
			
		// ====================================
		//		  GENERAL_ROAMING_STATE
		// ====================================
		class FindOZ : public ActionNode {
		public:
			// 상대방 팀의 점령지 중 아직 점령되지 않은 점령지를 찾고, 해당 점령지를 쳐다본다
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		class MoveToOZ : public ActionNode {
		public:
			// 점령지를 향해 달려간다.
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};


		// ====================================
		//		  GENERAL_DUELING_STATE
		// ====================================

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


