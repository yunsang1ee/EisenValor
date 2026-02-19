#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class BehaviorNode;

		// =================================
		//		 GENERAL_ROAMING_STATE
		// =================================
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

		// =================================
		//		 GENERAL_DUELING_STATE
		// =================================
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

		// =================================
		//		 GENERAL_STUN_STATE
		// =================================
		class GeneralStunState : public State {
			DECLARE_CREATE_FUNC(GeneralStunState)
		public:
			explicit GeneralStunState(FSM* const fsm);
			virtual ~GeneralStunState();

		public:	private:
			virtual void Enter(const float dt) override final;		
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// =================================
		//		 GENERAL_DEAD_STATE
		// =================================
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
