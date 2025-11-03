#pragma once
#include "Component.h"

namespace Server {
	namespace Contents {
		class FSM;

		class State {
		private:
			FSM*							m_fsm;
			uint8							m_type;

		protected:
			explicit State(const uint8 type);

		public:
			virtual ~State();

		public:
			virtual void Enter(const float dt) abstract;
			virtual void Exit(const float dt) abstract;

		public:
			virtual void Update(const float dt) abstract;

		public:
			uint8 GetStateType() const noexcept { return m_type; }

		public:
			void SetFSM(FSM* fsm) { m_fsm = fsm; }
			FSM* GetFSM() const { return m_fsm; }

		};

		// ============================================
		//					IDLE
		// ============================================
		class IdleState : public State {
		public:
			float enemyDetectionRange{};

		public:
			explicit IdleState(const uint8 type);
			virtual ~IdleState() = default;

		public:

		};


		// ============================================
		//					MOVE
		// ============================================
		class MoveState : public State {
		public:
			float combatRange;

		public:
			explicit MoveState(const uint8 type);
			virtual ~MoveState() = default;

		public:
		};


		// ============================================
		//					CHASE
		// ============================================
		class ChaseState : public State {
		public:
			float chaseSpeed;
			float combatRange;

		public:
			explicit ChaseState(const uint8 type);
			virtual ~ChaseState() = default;
		};


		// ============================================
		//					ATTACK
		// ============================================
		class AttackState : public State {
		public:
			float					combatRange;
			std::chrono::seconds	attackCycleTime;

		public:
			explicit AttackState(const uint8 type);
			virtual ~AttackState() = default;

		public:
		};


		// ============================================
		//					DEFENSE
		// ============================================
		class DefenseState : public State {
		public:
			explicit DefenseState(const uint8 type);
			virtual ~DefenseState() = default;

		public:
		};


		// ============================================
		//					DEAD
		// ============================================
		class DeadState : public State {
		public:
			explicit DeadState(const uint8 type);
			virtual ~DeadState() = default;

		public:
		};
	}
}

