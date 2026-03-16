#include "stdafxClient.h"
#include "FSMComponent.h"
#include "AnimationComponent.h"
#include "GameObject.h"
#include "GameObject.inl"
#include <Packets/Enums_generated.h>

void FSMComponent::OnUpdate(float deltaTime)
{
	// StatePool에서 현재 상태 로직을 빌려와 실행
	State* state = StatePool::GetState(m_curStateType);
	if (state)
	{
		// 애니메이션 종료 시 자동 전이
		if (state->HasExitTime() && state->GetNextStateOnEnd() != 0)
		{
			if (auto* anim = GetGameObject()->GetComponent<AnimationComponent>())
			{
				if (anim->IsAnimationEnd())
				{
					ChangeState(state->GetNextStateOnEnd());
					return; // 상태가 바뀌었으므로 이번 프레임 중단
				}
			}
		}

		// fsm 포인터를 인자로(자기자신)
		state->Update(this, deltaTime);
	}
}

void FSMComponent::SetServerState(uint8_t serverState)
{
	// 서버에서 오는 상태 기록
	m_serverState = serverState;

	// 서버에서 온 패킷 무시
	if (m_curStateType == FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY ||
		m_curStateType == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK ||
		m_curStateType == FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY ||
		m_curStateType == FB_ENUMS::PLAYER_STATE_TYPE_STUN)
	{
		return;
	}

	// 그 외 경우 서버 상태로 변경
	ChangeState(serverState);
}

void FSMComponent::ChangeState(uint8_t nextStateType)
{
	// 같은 상태로 다시 ChangeState가 불렸을 때 애니메이션을 다시 틀지 않도록 방어
	if (m_curStateType == nextStateType) return;

	// HasExitTime Check
	if (State* curState = StatePool::GetState(m_curStateType))
	{
		if (curState->HasExitTime())
		{
			// Animation Component Check
			if (auto* anim = GetGameObject()->GetComponent<AnimationComponent>())
			{
				// 안 끝났으면 전환 안 함
				if (!anim->IsAnimationEnd())
				{
					return;
				}
			}
		}
	}

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
