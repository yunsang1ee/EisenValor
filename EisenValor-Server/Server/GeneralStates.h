#pragma once

#include "State.h"

namespace Server {
	namespace Contents {

		// ==================================
		//		  GENERAL_IDLE_STATE
		// ==================================
		class GeneralIdleState : public State {
			 DECLARE_CREATE_FUNC(GeneralIdleState)

		private:
			float m_accDTForStaminaRecovery;

		public:
			GeneralIdleState();
			virtual ~GeneralIdleState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;

		public:
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		  GENERAL_MOVE_STATE
		// ==================================
		class GeneralMoveState : public State {
			DECLARE_CREATE_FUNC(GeneralMoveState)

		public:
			GeneralMoveState();
			virtual ~GeneralMoveState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;

		public:
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 GENERAL_PRE_DELAY_STATE
		// ==================================
		class GeneralPreDelayState : public State {
			DECLARE_CREATE_FUNC(GeneralPreDelayState)
		
		private:
			uint64 m_startFrame;

		public:
			GeneralPreDelayState();
			virtual ~GeneralPreDelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;

		public:
			virtual void Update(const float dt) override final;
		};


		// ==================================
		//		 GENERAL_ATTACK_STATE
		// ==================================
		class GeneralAttackState : public State {
			DECLARE_CREATE_FUNC(GeneralAttackState)
		public:
			GeneralAttackState();
			virtual ~GeneralAttackState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;

		public:
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 GENERAL_POST_DELAY_STATE
		// ==================================
		class GeneralPostDelayState : public State {
			DECLARE_CREATE_FUNC(GeneralPostDelayState)
		private:
			uint64 m_startFrame;

		public:
			GeneralPostDelayState();
			virtual ~GeneralPostDelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;

		public:
			virtual void Update(const float dt) override final;
		};


		// ==================================
		//		 GENERAL_STUN_STATE
		// ==================================
		class GeneralStunState : public State {
			DECLARE_CREATE_FUNC(GeneralStunState)
		private:
			uint64 m_startFrame;
			uint32 m_stunDuration;
		
		public:
			GeneralStunState();
			virtual ~GeneralStunState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

			void SetStunDuration(uint32 duration) { m_stunDuration = duration; }
		};


		// ==================================
		//		 GENERAL_DEAD_STATE
		// ==================================
		class GeneralDeadState : public State {
			DECLARE_CREATE_FUNC(GeneralDeadState)
		private:
			float m_accDTForRespawn;

		public:
			GeneralDeadState();
			virtual ~GeneralDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		};
	}
}

