#pragma once
#include "StatePool.h"

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

// Move
class GeneralMoveState : public State {
public:
	GeneralMoveState();
	virtual ~GeneralMoveState() = default;

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

// Defense
class GeneralDefenseState : public State {
public:
	GeneralDefenseState();
	virtual ~GeneralDefenseState() = default;

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

// Dead
class GeneralDeadState : public State {
public:
	GeneralDeadState();
	virtual ~GeneralDeadState() = default;

	virtual void Enter(FSMComponent* fsm) override;
	virtual void Update(FSMComponent* fsm, float dt) override;
	virtual void Exit(FSMComponent* fsm) override;
};
