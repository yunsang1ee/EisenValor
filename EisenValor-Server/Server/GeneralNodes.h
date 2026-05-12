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

		// 아직 점령되지 않은(UNOCCUPIED) 점령지 안에 있는지 확인한다.
		class IsInUnoccupiedZone : public ConditionNode {
		public:
			virtual bool Check(const float dt) override final;
		};

		// 모든 점령지가 점령된(OCCUPIED) 상태인지 확인한다.
		// 점령지가 하나도 없는 맵에서는 false를 반환한다.
		class AreAllZonesOccupied : public ConditionNode {
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
		// 매 사이클마다 [minSec, maxSec] 범위에서 새로운 쿨다운을 무작위로 결정한다.
		class IsAttackCooldownReady : public ConditionNode {
		public:
			explicit IsAttackCooldownReady(const float minSec = 3.f, const float maxSec = 7.f)
				: m_minSec{ minSec }, m_maxSec{ maxSec } {
			}
			virtual bool Check(const float dt) override final;
			virtual void Reset() override { m_acc = 0.f; m_cycleSec = -1.f; }
		private:
			float m_acc{};
			float m_cycleSec{ -1.f };
			float m_minSec;
			float m_maxSec;
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

		// 일정 간격마다 공격 방향을 무작위로 갱신하고 클라이언트에 표시 패킷을 브로드캐스트한다.
		// 항상 SUCCESS를 반환해 공격 사이클의 다른 노드 흐름을 막지 않는다.
		class RandomizeAttackDir : public ActionNode {
		public:
			explicit RandomizeAttackDir(const float intervalSec = 1.5f) : m_intervalSec{ intervalSec } {}
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
			virtual void Reset() override { m_acc = m_intervalSec; }

		private:
			float m_acc{};
			float m_intervalSec;
		};

		// 일정 간격마다 FWD/BWD/LFT/RGT 중 한 방향을 골라 [minDist, maxDist] 만큼
		// 그 방향으로 확실히 이동시킨다. 직전과 동일한 방향은 다시 뽑지 않아 변화감을 준다.
		class WanderAroundTarget : public ActionNode {
		public:
			explicit WanderAroundTarget(
				const float intervalSec = 1.2f,
				const float minDist = 1.0f,
				const float maxDist = 2.5f)
				: m_intervalSec{ intervalSec }, m_minDist{ minDist }, m_maxDist{ maxDist } {}
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
			virtual void Reset() override { m_acc = m_intervalSec; m_lastDirIdx = -1; }

		private:
			float m_acc{};
			float m_intervalSec;
			float m_minDist;
			float m_maxDist;
			int m_lastDirIdx{ -1 };
		};

		// 1회 공격을 수행한다 (사거리 안의 타겟에게 데미지 + 클라이언트에 공격 패킷 브로드캐스트).
		class Attack : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// 점령되지 않은 점령지를 찾아 NavAgent 목적지로 설정한다 (상태 변경 없음).
		class MoveToOZ : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		// NavAgent의 maxSpeed를 지정 값으로 설정한다 (이미 같으면 no-op).
		// 항상 SUCCESS를 반환해 Sequence 흐름을 막지 않는다.
		class SetMaxSpeed : public ActionNode {
		public:
			explicit SetMaxSpeed(const float maxSpeed) : m_maxSpeed{ maxSpeed } {}
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		private:
			float m_maxSpeed;
		};

		// 최초 1회 지정된 시간만큼 RUNNING을 반환해 트리 흐름을 막아두고,
		// 시간이 지나면 그 후로는 항상 FAIL을 반환해 형제 노드로 흐름을 넘긴다.
		// Reset()을 의도적으로 비워 상태 재진입에도 다시 발동하지 않는다.
		class WaitOnce : public ActionNode {
		public:
			explicit WaitOnce(const float durationSec) : m_durationSec{ durationSec } {}
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
			virtual void Reset() override {}

		private:
			float m_acc{ 0.f };
			float m_durationSec;
			bool  m_done{ false };
		};
	}
}


