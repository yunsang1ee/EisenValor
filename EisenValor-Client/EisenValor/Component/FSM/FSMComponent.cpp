#include "stdafxClient.h"
#include "FSMComponent.h"
#include "AnimationComponent.h"
#include "GameObject.h"
#include "GameObject.inl"
#include <Packets/Enums_generated.h>
#include "Util/GameConstants.h"

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

	// 병사 오프셋 처리
	uint8_t targetState = serverState;
	if (m_objType == FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER)
	{
		targetState += StateOffset::kSoldierOffset;
	}

	// 정책 기반 요청으로 변경 (공격 중 무시 등은 StatePolicy에서 처리)
	RequestState(StateRequestType::ForcedServerCorrection, targetState);
}

void FSMComponent::ChangeState(uint8_t nextStateType)
{
	// 기존 함수는 애니메이션 종료 대기가 기본값
	ChangeState(nextStateType, false);
}

void FSMComponent::ChangeState(uint8_t nextStateType, bool ignoreExitTime)
{
	//디버그
	//DEBUG_LOG_FMT("[FSM] ChangeState: {} -> {} (ObjType: {}, IgnoreExitTime: {})\n", (int)m_curStateType, (int)nextStateType, (int)m_objType, ignoreExitTime);

	// 같은 상태일 때 애니메이션을 다시 틀지 않도록 방어
	if (m_curStateType == nextStateType) return;

	// Dead 확인
	bool isNextDead = (nextStateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD) ||
					   nextStateType == static_cast<uint8_t>(FB_ENUMS::GENERAL_STATE_TYPE_DEAD) ||
					   nextStateType == (StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_DEAD)));

	// 종료 조건 체크 (ignoreExitTime이 false일 때만 수행)
	if (!ignoreExitTime && !isNextDead)
	{
		if (State* curState = StatePool::GetState(m_curStateType))
		{
			if (curState->HasExitTime())
			{
				// Animation Component Check
				if (auto* anim = GetGameObject()->GetComponent<AnimationComponent>())
				{
					if (!anim->IsAnimationEnd())
					{
						return;
					}
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

void FSMComponent::SetStance(uint8_t stance)
{
	if (m_stance == stance) return;
	m_stance = stance;

	for (auto& listener : m_stanceListeners)
	{
		if (listener) listener(m_stance);
	}
}

bool FSMComponent::RequestState(StateRequestType request, uint8_t targetStateOverride)
{
	static const PlayerStatePolicy playerPolicy;

	if (m_objType == FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER)
	{
		if (targetStateOverride == 0)
		{
			return false;
		}

		if (request != StateRequestType::ForcedServerCorrection)
		{
			return false;
		}

		ChangeState(targetStateOverride, true);
		return true;
	}

	StateTransitionDecision decision = playerPolicy.Resolve(*this, request, targetStateOverride);
	if (!decision.accepted)
	{
		return false;
	}

	ChangeState(decision.nextStateType, decision.ignoreExitTime);
	return true;
}
