#pragma once

#include "State.h"

namespace GameServer {
	namespace Contents {
		class BehaviorNode;

		class GeneralState : public State {
		public:
			explicit GeneralState(const uint8 stateType);
			virtual  ~GeneralState() = default;

		protected:
			std::unique_ptr<BehaviorNode>	m_root;
		};

		// =================================
		// 		  GENERAL_IDLE_STATE
		// =================================
		class GeneralIdleState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralIdleState)
		public:
			explicit GeneralIdleState(const std::shared_ptr<General>& owner);
			virtual ~GeneralIdleState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// =================================
		//		GENERAL_WALK_STATE
		// =================================
		class GeneralWalkState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralWalkState)
		public:
			explicit GeneralWalkState();
			virtual ~GeneralWalkState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// =================================
		// 		 GENERAL_RUN_STATE
		// =================================
		class GeneralRunState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralRunState)
		public:
			explicit GeneralRunState();
			virtual ~GeneralRunState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// =================================
		//		GENERAL_ATTACK_STATE
		// =================================
		class GeneralAttackState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralAttackState)
		public:
			explicit GeneralAttackState();
			virtual ~GeneralAttackState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// =================================
		//		GENERAL_STUN_STATE
		// =================================
		class GeneralStunState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralStunState)
		public:
			explicit GeneralStunState();
			virtual ~GeneralStunState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// =================================
		// GENERAL_DEAD_STATE
		// =================================
		class GeneralDeadState : public GeneralState {
			DECLARE_CREATE_FUNC(GeneralDeadState)
		public:
			explicit GeneralDeadState();
			virtual ~GeneralDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// =================================
		//		 GENERAL_ROAMING_STATE
		// =================================
		//class GeneralRoamingState : public GeneralState {
		//	DECLARE_CREATE_FUNC(GeneralRoamingState)
		//public:
		//	explicit GeneralRoamingState();
		//	virtual ~GeneralRoamingState();

		//public:
		//	virtual void Enter(const float dt) override final;
		//	virtual void Exit(const float dt) override final;
		//	virtual void Update(const float dt) override final;

		//private:
		//	void RecoveryStamina(const float dt);
		//	void FindGeneral(const float dt);
		//
		//private:
		//	std::unique_ptr<BehaviorNode>	m_root;
		//	float							m_accDTForStaminaRecovery;
		//};

		// =================================
		//		 GENERAL_DUELING_STATE
		// =================================
		//class GeneralDuelingState : public GeneralState {
		//	DECLARE_CREATE_FUNC(GeneralDuelingState)
		//public:
		//	explicit GeneralDuelingState();
		//	virtual ~GeneralDuelingState();

		//public:
		//	virtual void Enter(const float dt) override final;
		//	virtual void Exit(const float dt) override final;
		//	virtual void Update(const float dt) override final;

		//private:
		//	std::unique_ptr<BehaviorNode> m_root;
		//};

		// =================================
		//		 GENERAL_STUN_STATE
		// =================================
		//class GeneralStunState : public GeneralState {
		//	DECLARE_CREATE_FUNC(GeneralStunState)
		//public:
		//	explicit GeneralStunState();
		//	virtual ~GeneralStunState();

		//public:
		//	virtual void Enter(const float dt) override final;		
		//	virtual void Exit(const float dt) override final;
		//	virtual void Update(const float dt) override final;
		//
		//private:
		//	float m_accDTForStunState;
		//};

		//// =================================
		////		 GENERAL_DEAD_STATE
		//// =================================
		//class GeneralDeadState : public GeneralState {
		//	DECLARE_CREATE_FUNC(GeneralDeadState)
		//public:
		//	explicit GeneralDeadState();
		//	virtual ~GeneralDeadState();

		//public:
		//	virtual void Enter(const float dt) override final;
		//	virtual void Exit(const float dt) override final;
		//	virtual void Update(const float dt) override final;

		//private:
		//	float m_accDTForRespawn;
		//};

	}
}
