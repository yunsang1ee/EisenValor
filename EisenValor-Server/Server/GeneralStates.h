#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class BehaviorNode;

		class GeneralState : public State {
		public:
			explicit GeneralState(const uint8 stateType, FSM* const fsm);
			virtual  ~GeneralState() = default;

		protected:
			std::unique_ptr<BehaviorNode>	m_root;
			BehaviorTree*					m_bt;
			General*						m_owner;
			std::shared_ptr<GameWorld>		m_gameWorld;
		};

		// =================================
		//		 GENERAL_ROAMING_STATE
		// =================================
		class GeneralRoamingState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralRoamingState)
		public:
			explicit GeneralRoamingState(FSM* const fsm);
			virtual ~GeneralRoamingState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			void RecoveryStamina(const float dt);
			void FindGeneral(const float dt);
		
		private:
			std::unique_ptr<BehaviorNode>	m_root;
			float							m_accDTForStaminaRecovery;
		};

		// =================================
		//		 GENERAL_DUELING_STATE
		// =================================
		class GeneralDuelingState : public GeneralState {
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
		class GeneralStunState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralStunState)
		public:
			explicit GeneralStunState(FSM* const fsm);
			virtual ~GeneralStunState();

		public:
			virtual void Enter(const float dt) override final;		
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		
		private:
			float m_accDTForStunState;
		};

		// =================================
		//		 GENERAL_DEAD_STATE
		// =================================
		class GeneralDeadState : public GeneralState {
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
