#include "stdafxClient.h"
#include "FSMComponent.h"

void FSMComponent::OnUpdate(float deltaTime)
{
	// StatePool에서 현재 상태 로직을 빌려와 실행
	State* state = StatePool::GetState(m_curStateType);
	if (state)
	{
		// handle이 인자
		state->Update(GetHandle(), deltaTime);
	}
}

void FSMComponent::ChangeState(uint8_t nextStateType, float dt)
{
	if (m_curStateType == nextStateType) return;

	// 1. 기존 상태 Exit
	if (State* prevState = StatePool::GetState(m_curStateType))
	{
		prevState->Exit(GetHandle(), dt);
	}

	// 2. 새 상태 검증 및 Enter
	if (State* nextState = StatePool::GetState(nextStateType))
	{
		m_curStateType = nextStateType;
		nextState->Enter(GetHandle(), dt);

		// 3. 등록된 리스너들에게 알림
		for (auto& listener : m_listeners)
		{
			if (listener) listener(m_curStateType);
		}
	}
}
