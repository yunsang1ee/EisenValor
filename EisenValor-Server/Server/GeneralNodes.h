#pragma once
#include "BehaviorNode.h"

namespace GameServer {
	namespace Contents {
		// =====================================================
		//				     CONDITION NODES
		// =====================================================
		// 점령지 안에 있는지 확인한다.
		class IsInOccupationZone : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
		};

		// 죽은지 일정 시간이 지났는지 확인한다.
		class IsRespawnReady : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
			void Reset() { m_accDTForRespawn = 0.f; }

		private:
			float m_accDTForRespawn{};
		};

		// 유효한 타겟을 보유하고 있는지 확인한다.
		class HasTarget : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
		};

		// 타겟이 사라졌거나 더 이상 유효하지 않은지 확인한다.
		class IsTargetLost : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
		};

		// 타겟이 적 감지 범위 안에 있는지 확인한다.
		class IsTargetInDetectionRange : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
		};

		// 타겟이 전투 범위 안에 있는지 확인한다.
		class IsTargetInCombatRange : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
		};

		// 타겟이 실제 공격 히트박스(반경/각도) 안에 있는지 확인한다.
		class IsTargetInAttackRange : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
		};

		// 공격 쿨타임이 차서 다시 공격할 수 있는지 확인한다.
		class IsAttackCooldownReady : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
			virtual void Reset() override { m_acc = 0.f; }
		private:
			float m_acc{};
		};

		// 스턴 지속시간이 끝났는지 확인한다.
		class IsStunOver : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
			virtual void Reset() override { m_acc = 0.f; }
		private:
			float m_acc{};
		};

		// =====================================================
		// 				    ACTION NODES
		// =====================================================
		// FSM의 상태를 변경한다.
		class ChangeState : public ActionNode {
		public:
			ChangeState(const FB_ENUMS::GENERAL_STATE_TYPE stateType) : m_stateType{ stateType } {}
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;

		private:
			FB_ENUMS::GENERAL_STATE_TYPE m_stateType;
		};

		// 리스폰 위치로 이동한다.
		class Respawn : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// 감지 범위 내의 가장 가까운 적을 찾아 타겟으로 설정한다.
		class FindEnemy : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// 현재 타겟을 해제한다.
		class ClearTarget : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// 타겟 위치로 NavAgent 목적지를 갱신한다.
		class MoveToTarget : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// NavAgent 이동을 정지시킨다.
		class StopMoving : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// 타겟을 향해 회전한다.
		class LookAtTarget : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// 자세(Stance)를 설정한다.
		class SetStance : public ActionNode {
		public:
			explicit SetStance(const FB_ENUMS::GENERAL_STANCE_TYPE stance) : m_stance{ stance } {}
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		private:
			FB_ENUMS::GENERAL_STANCE_TYPE m_stance;
		};

		// 1회 공격을 수행한다 (사거리 안의 타겟에게 데미지).
		class Attack : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// 점령되지 않은 점령지를 찾아 NavAgent 목적지로 설정한다 (상태 변경 없음).
		class MoveToOZ : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};
	}
}


