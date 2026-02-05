#pragma once
#include "StatePool.h"

// idle
// 서버 로직에 맞게 이식
// 데이터 없이 로직만 수행

class GeneralIdleState : public State {
public:
	GeneralIdleState();
	virtual ~GeneralIdleState() = default;

	virtual void Enter(HandleOf<FSMComponent> fsmHandle, float dt) override;
	virtual void Update(HandleOf<FSMComponent> fsmHandle, float dt) override;
	virtual void Exit(HandleOf<FSMComponent> fsmHandle, float dt) override;
};
