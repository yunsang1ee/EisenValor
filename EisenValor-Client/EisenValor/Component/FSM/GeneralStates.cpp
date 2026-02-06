#include "stdafxClient.h"
#include "GeneralStates.h"
#include "FSMComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include <GameObject.h>
#include <Packets/Enums_generated.h>

// ==================================
//		  GENERAL_IDLE_STATE
// ==================================
GeneralIdleState::GeneralIdleState(): State(FB_ENUMS::GENERAL_STATE_TYPE_IDLE)
{
}

void GeneralIdleState::Enter(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] IDLE Enter (Subject: {})\n", fsm->GetHandle().GetValue());
}

void GeneralIdleState::Update(FSMComponent* fsm, float dt)
{
}

void GeneralIdleState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] IDLE Exit\n");
}

// ==================================
//		  GENERAL_MOVE_STATE
// ==================================
GeneralMoveState::GeneralMoveState(): State(FB_ENUMS::GENERAL_STATE_TYPE_MOVE)
{
}

void GeneralMoveState::Enter(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] MOVE Enter (Subject: {})\n", fsm->GetHandle().GetValue());
}

void GeneralMoveState::Update(FSMComponent* fsm, float dt)
{
}

void GeneralMoveState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] MOVE Exit\n");
}

// ==================================
//		 GENERAL_PRE_DELAY_STATE
// ==================================
GeneralPreDelayState::GeneralPreDelayState() : State(FB_ENUMS::GENERAL_STATE_TYPE_PRE_DELAY)
{
}

void GeneralPreDelayState::Enter(FSMComponent* fsm)
{
	if (fsm)
	{
		fsm->SetStateTimer(0.0f);
		DEBUG_LOG_FMT("[FSM] PRE_DELAY Enter\n");
	}
}

void GeneralPreDelayState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	auto* owner = fsm->GetGameObject();
	if (!owner) return;

	auto* battleUI = owner->GetComponent<BattleUIControllerComponent>();
	if (!battleUI) return;

	fsm->AddStateTimer(dt);

	// BattleUI에서 실제 공격 타입을 가져옴
	auto attackTypeOpt = battleUI->GetCurrentAttackType();
	if (!attackTypeOpt.has_value()) return;

	GENERAL_ATTACK_TYPE type = attackTypeOpt.value();

	// 약공격: 10FPS, 강공격: 20FPS
	float targetTime = (type == GENERAL_ATTACK_TYPE_HEAVY) ? (20.0f / 60.0f) : (10.0f / 60.0f);

	if (fsm->GetStateTimer() >= targetTime)
	{
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ATTACK);
	}
}

void GeneralPreDelayState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] PRE_DELAY Exit\n");
}

// ==================================
//		  GENERAL_ATTACK_STATE
// ==================================
GeneralAttackState::GeneralAttackState() : State(FB_ENUMS::GENERAL_STATE_TYPE_ATTACK)
{
}

void GeneralAttackState::Enter(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] ATTACK Enter!\n");
	fsm->SetStateTimer(0.0f);
}

void GeneralAttackState::Update(FSMComponent* fsm, float dt)
{
	fsm->AddStateTimer(dt);
	// 임시로 0.1초 후 IDLE 복귀 (나중에 POST_DELAY로 교체)
	if (fsm->GetStateTimer() >= 0.1f) 
	{
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE);
	}
}

void GeneralAttackState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] ATTACK Exit\n");
}
