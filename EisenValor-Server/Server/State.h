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


		class IdleState : public State {
		public:
			float enemyDetectionRange{};

		public:
			explicit IdleState(const uint8 type);
			virtual ~IdleState() = default;

		public:

		};

		class RunState : public State {
		public:
			float combatRange;

		public:
			explicit RunState(const uint8 type);
			virtual ~RunState() = default;

		public:
		};

		class AttackState : public State {
		public:
			float combatRange;
			std::chrono::seconds attackCycleTime;

		public:
			explicit AttackState(const uint8 type);
			virtual ~AttackState() = default;

		public:
		};

		class DefenseState : public State {
		public:
			explicit DefenseState(const uint8 type);
			virtual ~DefenseState() = default;

		public:
		};

		class DeadState : public State {
		public:
			explicit DeadState(const uint8 type);
			virtual ~DeadState() = default;

		public:
		};
	}
}

