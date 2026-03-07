#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class GameObjectFactory;
		class GameObject;

		// ============================================
		//					IDLE
		// ============================================
		class SoldierSpawnState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierSpawnState)
		private:
			explicit SoldierSpawnState();
			virtual ~SoldierSpawnState();
		
		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDT;
			friend class GameObjectFactory;
		};

		// ============================================
		//					MOVE
		// ============================================
		class SoldierMoveState : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierMoveState)
		private:
			explicit SoldierMoveState(const float viewRange);
			virtual ~SoldierMoveState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float				m_viewRangeSq{};
			float				m_accDTForSearch;
			std::queue<Vec3>	m_wayPoints;

			friend class GameObjectFactory;
		};

		class SoldierSearchState : public State {
			DECLARE_CREATE_FUNC(SoldierSearchState)
		private:
			explicit SoldierSearchState(const float attackRange);
			virtual ~SoldierSearchState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_attackRangeSq;
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

		class SoldierDeadState : public State {
			DECLARE_CREATE_FUNC(SoldierDeadState)

		private:
			explicit SoldierDeadState();
			virtual ~SoldierDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDT;
			friend class GameObjectFactory;
		};

	}
}
