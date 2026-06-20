#pragma once
#include "StatePool.h"

// ==================================
//            GENERAL_STATES
// ==================================
// idle
// 서버 로직에 맞게 이식
// 데이터 없이 로직만 수행
class GeneralIdleState : public State {
public:
	GeneralIdleState();
	virtual ~GeneralIdleState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

class GeneralWalkState : public State {
	public:
	GeneralWalkState();
	virtual ~GeneralWalkState() = default;
	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

class GeneralRunState : public State {
	public:
	GeneralRunState();
	virtual ~GeneralRunState() = default;
	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Dodge
class GeneralDodgeState : public State {
public:
	GeneralDodgeState();
	virtual ~GeneralDodgeState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Roll
class GeneralRollState : public State {
public:
	GeneralRollState();
	virtual ~GeneralRollState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// PreDelay
class GeneralPreDelayState : public State {
public:
	GeneralPreDelayState();
	virtual ~GeneralPreDelayState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Attack
class GeneralAttackState : public State {
public:
	GeneralAttackState();
	virtual ~GeneralAttackState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// PostDelay
class GeneralPostDelayState : public State {
public:
	GeneralPostDelayState();
	virtual ~GeneralPostDelayState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Stun
class GeneralStunState : public State {
public:
	GeneralStunState();
	virtual ~GeneralStunState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Guard
class GeneralGuardState : public State {
public:
	GeneralGuardState();
	virtual ~GeneralGuardState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Dead
class GeneralDeadState : public State {
public:
	GeneralDeadState();
	virtual ~GeneralDeadState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// ==================================
//            SOLDIER STATES
// ==================================

// Soldier Idle
class SoldierIdleState : public State {
public:
	SoldierIdleState();
	virtual ~SoldierIdleState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Soldier Move
class SoldierMoveState : public State {
public:
	SoldierMoveState();
	virtual ~SoldierMoveState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Soldier Stun
class SoldierStunState : public State {
public:
	SoldierStunState();
	virtual ~SoldierStunState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Soldier Dead
class SoldierDeadState : public State {
public:
	SoldierDeadState();
	virtual ~SoldierDeadState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Soldier Chase
class SoldierChaseState : public State {
public:
	SoldierChaseState();
	virtual ~SoldierChaseState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Soldier Attack
class SoldierAttackState : public State {
public:
	SoldierAttackState();
	virtual ~SoldierAttackState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};
