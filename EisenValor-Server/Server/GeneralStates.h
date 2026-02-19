#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class BehaviorNode;

		class GeneralRoamingState : public State {
			DECLARE_CREATE_FUNC(GeneralRoamingState)
		public:
			explicit GeneralRoamingState(FSM* const fsm);
			virtual ~GeneralRoamingState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		
		private:
			std::unique_ptr<BehaviorNode> m_root;
		};

		class GeneralDuelingState : public State {
			DECLARE_CREATE_FUNC(GeneralDuelingState)
		public:
			explicit GeneralDuelingState(FSM* const fsm);
			virtual ~GeneralDuelingState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			std::unique_ptr<BehaviorNode> m_root;
		};

		class GeneralDeadState : public State {
			DECLARE_CREATE_FUNC(GeneralDeadState)
		public:
			explicit GeneralDeadState(FSM* const fsm);
			virtual ~GeneralDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDTForRespawn;
		};

	}
}
