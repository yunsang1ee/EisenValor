#include "stdafxClient.h"
#include "FSMComponent.h"

void FSMComponent::OnUpdate(float deltaTime)
{
	// StatePool에서 현재 상태 로직을 빌려와 실행
	State* state = StatePool::GetState(m_curStateType);
	if (state)
	{
		// fsm 포인터를 인자로(자기자신)
		state->Update(this, deltaTime);
	}
}

void FSMComponent::ChangeState(uint8_t nextStateType)
{
	if (m_curStateType == nextStateType) return;

	// 1. 기존 상태 Exit
	if (State* prevState = StatePool::GetState(m_curStateType))
	{
		prevState->Exit(this);
	}

	// 2. 새 상태 검증 및 Enter
	if (State* nextState = StatePool::GetState(nextStateType))
	{
		m_curStateType = nextStateType;
		nextState->Enter(this);

		// 3. 등록된 리스너들에게 알림
		for (auto& listener : m_listeners)
		{
			if (listener) listener(m_curStateType);
		}
	}
}
