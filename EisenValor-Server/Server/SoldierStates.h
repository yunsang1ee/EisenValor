#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class GameObject;

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

		class SoldierRunState : public RunState {
		public:
			SoldierRunState();
			virtual ~SoldierRunState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;
	
		public:
			virtual void Update(const float dt) override;

		};

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
