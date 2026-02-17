#pragma once

#include "State.h"

namespace Server {
	namespace Contents {

		// ==================================
		//		  GENERAL_IDLE_STATE
		// ==================================
		class PlayerIdleState : public State {
			 DECLARE_CREATE_FUNC(PlayerIdleState)
		private:
			float m_accDTForStaminaRecovery;
			float m_accDTForExhaustedRecovery;

		private:
			PlayerIdleState();
			virtual ~PlayerIdleState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		  GENERAL_MOVE_STATE
		// ==================================
		class PlayerMoveState : public State {
			DECLARE_CREATE_FUNC(PlayerMoveState)
		private:
			PlayerMoveState();
			virtual ~PlayerMoveState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 GENERAL_PRE_DELAY_STATE
		// ==================================
		class PlayerPredelayState : public State {
			DECLARE_CREATE_FUNC(PlayerPredelayState)
		private:
			uint64 m_startFrame;

		private:
			PlayerPredelayState();
			virtual ~PlayerPredelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 GENERAL_ATTACK_STATE
		// ==================================
		class PlayerAttackState : public State {
			DECLARE_CREATE_FUNC(PlayerAttackState)
		private:
			PlayerAttackState();
			virtual ~PlayerAttackState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 GENERAL_POST_DELAY_STATE
		// ==================================
		class PlayerPostdelayState : public State {
			DECLARE_CREATE_FUNC(PlayerPostdelayState)
		private:
			uint64 m_startFrame;

		private:
			PlayerPostdelayState();
			virtual ~PlayerPostdelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};


		// ==================================
		//		 GENERAL_STUN_STATE
		// ==================================
		class PlayerStunState : public State {
			DECLARE_CREATE_FUNC(PlayerStunState)
		private:
			uint64 m_startFrame;
			uint32 m_stunDuration;
		
		private:
			PlayerStunState();
			virtual ~PlayerStunState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

			void SetStunDuration(uint32 duration) { m_stunDuration = duration; }
		};


		// ==================================
		//		 GENERAL_DEAD_STATE
		// ==================================
		class PlayerDeadState : public State {
			DECLARE_CREATE_FUNC(PlayerDeadState)
		private:
			float m_accDTForRespawn;

		private:
			PlayerDeadState();
			virtual ~PlayerDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		};
	}
}

