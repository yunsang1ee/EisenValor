#pragma once
#include "StatePool.h"

// ==================================
//            PLAYER_STATES
// ==================================
// idle
// 서버 로직에 맞게 이식
// 데이터 없이 로직만 수행
class PlayerlIdleState : public State {
public:
	PlayerlIdleState();
	virtual ~PlayerlIdleState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Move
class PlayerMoveState : public State {
public:
	PlayerMoveState();
	virtual ~PlayerMoveState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// PreDelay
class PlayerPreDelayState : public State {
public:
	PlayerPreDelayState();
	virtual ~PlayerPreDelayState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Attack
class PlayerAttackState : public State {
public:
	PlayerAttackState();
	virtual ~PlayerAttackState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// PostDelay
class PlayerPostDelayState : public State {
public:
	PlayerPostDelayState();
	virtual ~PlayerPostDelayState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Defense
class PlayerDefenseState : public State {
public:
	PlayerDefenseState();
	virtual ~PlayerDefenseState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Stun
class PlayerStunState : public State {
public:
	PlayerStunState();
	virtual ~PlayerStunState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};

// Dead
class PlayerDeadState : public State {
public:
	PlayerDeadState();
	virtual ~PlayerDeadState() = default;

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

