#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class GameObjectFactory;
		class GameObject;

		// ============================================
		//					IDLE
		// ============================================
		class SoldierIdleState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierIdleState)
		private:
			explicit SoldierIdleState(const float enemyDetectionRange);
			virtual ~SoldierIdleState();
		
		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_enemyDetectionRangeSq{};
		};


		// ============================================
		//					MOVE
		// ============================================
		class SoldierMoveState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierMoveState)
		private:
			SoldierMoveState();
			virtual ~SoldierMoveState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
			friend class GameObjectFactory;
		};


		// ============================================
		//					CHASE
		// ============================================
		class SoldierChaseState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierChaseState)
		private:
			explicit SoldierChaseState(const float chaseSpeed, const float combatRange);
			virtual ~SoldierChaseState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
			friend class GameObjectFactory;

		private:
			static constexpr float COMBAT_PROB{ 0.7f };
			float m_chaseSpeed;
			float m_combatRange;

		};


		// ============================================
		//					ATTACK
		// ============================================
		class SoldierAttackState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierAttackState)
		private:
			explicit SoldierAttackState(const float combatRange, const std::chrono::seconds attackCycleTime);
			virtual ~SoldierAttackState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
			friend class GameObjectFactory;

		private:
			float					m_accDt;
			float					m_combatRange;
			std::chrono::seconds	m_attackCycleTime;
			static constexpr float ATTACK_PROB{ 0.7f };
		};


		// ============================================
		//					DEFENSE
		// ============================================
		class SoldierDefenseState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierDefenseState)
		private:
			SoldierDefenseState();
			virtual ~SoldierDefenseState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
			friend class GameObjectFactory;

		private:
			float					m_accDT;
			static constexpr auto	DEFENSE_TIME = 1s;
			static constexpr float	ATTACK_PROB{ 0.7f };
		};


		// ============================================
		//					DAMAGED
		// ============================================
		class SoldierDamagedState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierDamagedState)
		private:
			explicit SoldierDamagedState(const float stunTime);
			virtual ~SoldierDamagedState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		
		public:
			void SetStunTime(const float stunTime){ m_stunTime = stunTime; }
			friend class GameObjectFactory;

		private:
			float m_stunTime;
			float m_accForStun;

		};

	}
}
