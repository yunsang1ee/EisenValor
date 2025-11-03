#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class GameObject;

		// ============================================
		//					IDLE
		// ============================================
		class SoldierIdleState : public IdleState {
		public:
			SoldierIdleState();
			virtual ~SoldierIdleState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;

		public:
			virtual void Update(const float dt) override;
		};


		// ============================================
		//					MOVE
		// ============================================
		class SoldierMoveState : public MoveState {
		public:
			SoldierMoveState();
			virtual ~SoldierMoveState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;

		public:
			virtual void Update(const float dt) override;

		};


		// ============================================
		//					CHASE
		// ============================================
		class SoldierChaseState : public ChaseState {
		public:
			SoldierChaseState();
			virtual ~SoldierChaseState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;

		public:
			virtual void Update(const float dt) override;
		};


		// ============================================
		//					ATTACK
		// ============================================
		class SoldierAttackState : public AttackState {
		public:
			float					m_accDt{ 0.f };

		public:
			SoldierAttackState();
			virtual ~SoldierAttackState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;

		public:
			virtual void Update(const float dt) override;


		};


		// ============================================
		//					DEFENSE
		// ============================================
		class SoldierDefenseState : public DefenseState {
		public:
			float					m_accDT{ 0.f };
			static constexpr auto	DEFENSE_TIME = 1s;

		public:
			SoldierDefenseState();
			virtual ~SoldierDefenseState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;

		public:
			virtual void Update(const float dt) override;
		};
	}
}
