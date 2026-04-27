#pragma once

#include "State.h"

namespace GameServer {
	namespace Contents {
		class GameObjectFactory;
		class GameObject;

		// TOOD: 구조체 형태의 데이터로 만들어서 각 State 클래스의 생성자에 전달하는 형태로 변경해야함.

		// ============================================
		//					IDLE
		// ============================================
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
			static constexpr float SPAWN_DELAY{ 2.f };
			float		m_accDT;
			bool		m_isSpawned;
			friend class GameObjectFactory;
		};

		// ============================================
		//					MOVE
		// ============================================
		class SoldierMoveState final : public State {
		private:
			DECLARE_CREATE_FUNC(SoldierMoveState)
		private:
			explicit SoldierMoveState(const float viewRange, const Vec3& destPos);
			virtual ~SoldierMoveState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;

		private:
			static constexpr float SEARCH_INTERVAL{ 0.1f };
			float				m_viewRangeSq{};
			float				m_accDTForSearch;
			Vec3				m_destPos;

			friend class GameObjectFactory;
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
			float m_chaseTransitionRangeSq;
			float m_accDTForChase;
			static constexpr float CHASE_UPDATE_INTERVAL = 0.2f; // 0.2초마다 경로 갱신
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
			float	m_accDTForAttack;
			float	m_accDTForDamage;

			float	m_attackRangeSq;
			bool	m_attackStarted;

			friend class GameObjectFactory;
		};

		// ============================================
		// 					  DEAD
		// ============================================
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
