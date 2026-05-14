#pragma once

#include "State.h"

namespace GameServer {
	namespace Contents {

		// ==================================
		//		  PLAYER_IDLE_STATE
		// ==================================
		class PlayerIdleState : public State {
			 DECLARE_CREATE_FUNC(PlayerIdleState)
		private:
			PlayerIdleState();
			virtual ~PlayerIdleState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDTForStaminaRecovery;
			float m_accDTForExhaustedRecovery;
		};

		// ==================================
		// 		 PLAYER_WALK_STATE
		// ==================================
		class PlayerWalkState : public State {
			DECLARE_CREATE_FUNC(PlayerWalkState)

		private:
			PlayerWalkState();
			virtual ~PlayerWalkState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		// 		 PLAYER_RUN_STATE
		// ==================================
		class PlayerRunState : public State {
			DECLARE_CREATE_FUNC(PlayerRunState)
		private:
			PlayerRunState();
			virtual ~PlayerRunState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		// ==================================
		//		 PLAYER_PRE_DELAY_STATE
		// ==================================
		class PlayerPredelayState : public State {
			DECLARE_CREATE_FUNC(PlayerPredelayState)
		private:
			PlayerPredelayState();
			virtual ~PlayerPredelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDTForPreDelay;
		
		};

		// ==================================
		//		PLAYER_ATTACK_STATE
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

		private:
			float m_accDT{ 0.f };
			bool  m_hitFired{ false };
		};

		// ==================================
		//		 PLAYER_POST_DELAY_STATE
		// ==================================
		class PlayerPostdelayState : public State {
			DECLARE_CREATE_FUNC(PlayerPostdelayState)
		private:
			PlayerPostdelayState();
			virtual ~PlayerPostdelayState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};


		// ==================================
		//		 PLAYER_STUN_STATE
		// ==================================
		class PlayerStunState : public State {
			DECLARE_CREATE_FUNC(PlayerStunState)
		private:
			PlayerStunState();
			virtual ~PlayerStunState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float	m_accDTForStun;
		};


		// ==================================
		//		 PLAYER_DEAD_STATE
		// ==================================
		class PlayerDeadState : public State {
			DECLARE_CREATE_FUNC(PlayerDeadState)
		private:
			PlayerDeadState();
			virtual ~PlayerDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDTForRespawn;
		};
	}
}

