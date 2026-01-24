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

		private:
			GeneralIdleState();
			virtual ~GeneralIdleState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		  GENERAL_MOVE_STATE
		// ==================================
		class GeneralMoveState : public State {
			DECLARE_CREATE_FUNC(GeneralMoveState)
		private:
			GeneralMoveState();
			virtual ~GeneralMoveState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 GENERAL_PRE_DELAY_STATE
		// ==================================
		class GeneralPreDelayState : public State {
			DECLARE_CREATE_FUNC(GeneralPreDelayState)
		private:
			uint64 m_startFrame;

		private:
			GeneralPreDelayState();
			virtual ~GeneralPreDelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};


		// ==================================
		//		 GENERAL_ATTACK_STATE
		// ==================================
		class GeneralAttackState : public State {
			DECLARE_CREATE_FUNC(GeneralAttackState)
		private:
			GeneralAttackState();
			virtual ~GeneralAttackState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 GENERAL_POST_DELAY_STATE
		// ==================================
		class GeneralPostDelayState : public State {
			DECLARE_CREATE_FUNC(GeneralPostDelayState)
		private:
			uint64 m_startFrame;

		private:
			GeneralPostDelayState();
			virtual ~GeneralPostDelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
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
		
		private:
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

		private:
			GeneralDeadState();
			virtual ~GeneralDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		};
	}
}

