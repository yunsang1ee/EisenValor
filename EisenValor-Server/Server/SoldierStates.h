#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class GameObjectFactory;
		class GameObject;

		// ============================================
		//					IDLE
		// ============================================
		class SoldierSpawnState final : public State {
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

		class SoldierIdleState final : public State {
			DECLARE_CREATE_FUNC(SoldierIdleState)

		private:
			explicit SoldierIdleState();
			virtual ~SoldierIdleState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			friend class GameObjectFactory;
		};

		// ============================================
		//					MOVE
		// ============================================
		class SoldierMoveState final : public State {
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
			std::vector<Vec3>	m_wayPoints;
			uint32				m_currentWaypointIndex;

			friend class GameObjectFactory;
		};

		class SoldierSearchState final : public State {
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
		class SoldierChaseState final : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierChaseState)
		private:
			explicit SoldierChaseState(const float chaseRange, const float attackRange);
			virtual ~SoldierChaseState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
			friend class GameObjectFactory;

		private:
			float m_attackRangeSq;
			float m_chaseRangeSq;

		};


		// ============================================
		//					ATTACK
		// ============================================
		class SoldierAttackState final : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierAttackState)
		private:
			explicit SoldierAttackState(const float attackRange);
			virtual ~SoldierAttackState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDTForAttack;
			float m_attackRangeSq;

			friend class GameObjectFactory;
		};

		class SoldierDeadState  final : public State {
			DECLARE_CREATE_FUNC(SoldierDeadState)

		private:
			explicit SoldierDeadState(const float deadAnimTime);
			virtual ~SoldierDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			float m_accDT;
			float m_deadAnimTime;
			friend class GameObjectFactory;
		};

	}
}
