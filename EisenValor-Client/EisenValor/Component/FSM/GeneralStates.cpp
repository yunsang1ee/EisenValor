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

	fsm->AddStateTimer(dt);

	// FSM에 저장된 공격 타입을 사용 (버튼을 떼도 유지됨)
	GENERAL_ATTACK_TYPE type = static_cast<GENERAL_ATTACK_TYPE>(fsm->GetCurAttackType());

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
	// 임시로 1.0초 후 POST_DELAY 전환
	if (fsm->GetStateTimer() >= 1.0f) 
	{
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_POST_DELAY);
	}
}

void GeneralAttackState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] ATTACK Exit\n");
}

// ==================================
//		 GENERAL_POST_DELAY_STATE
// ==================================
GeneralPostDelayState::GeneralPostDelayState() : State(FB_ENUMS::GENERAL_STATE_TYPE_POST_DELAY)
{
}

void GeneralPostDelayState::Enter(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] POST_DELAY Enter\n");
	fsm->SetStateTimer(0.0f);
}

void GeneralPostDelayState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	fsm->AddStateTimer(dt);

	// FSM에 저장된 공격 타입을 사용
	GENERAL_ATTACK_TYPE type = static_cast<GENERAL_ATTACK_TYPE>(fsm->GetCurAttackType());

	// 약공격: 5FPS, 강공격: 10FPS
	float targetTime = (type == GENERAL_ATTACK_TYPE_HEAVY) ? (10.0f / 60.0f) : (5.0f / 60.0f);

	if (fsm->GetStateTimer() >= targetTime)
	{
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE);
	}
}

void GeneralPostDelayState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] POST_DELAY Exit\n");
}

// ==================================
//		  GENERAL_DEFENSE_STATE
// ==================================
GeneralDefenseState::GeneralDefenseState() : State(FB_ENUMS::GENERAL_STATE_TYPE_DEFENSE)
{
}

void GeneralDefenseState::Enter(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] DEFENSE Enter (Block Success!)\n");
	fsm->SetStateTimer(0.0f);
}

void GeneralDefenseState::Update(FSMComponent* fsm, float dt)
{
	fsm->AddStateTimer(dt);
	// 방어 성공 연출 시간 (1초)
	if (fsm->GetStateTimer() >= 1.0f)
	{
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE);
	}
}

void GeneralDefenseState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] DEFENSE Exit\n");
}

// ==================================
//		  GENERAL_STUN_STATE
// ==================================
GeneralStunState::GeneralStunState() : State(FB_ENUMS::GENERAL_STATE_TYPE_STUN)
{
}

void GeneralStunState::Enter(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] STUN Enter (Hit!)\n");
	fsm->SetStateTimer(0.0f);
}

void GeneralStunState::Update(FSMComponent* fsm, float dt)
{
	fsm->AddStateTimer(dt);
	// 피격 경직 시간 (1초)
	if (fsm->GetStateTimer() >= 1.0f)
	{
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE);
	}
}

void GeneralStunState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] STUN Exit\n");
}

// ==================================
//		  GENERAL_DEAD_STATE
// ==================================
GeneralDeadState::GeneralDeadState() : State(FB_ENUMS::GENERAL_STATE_TYPE_DEAD)
{
}

void GeneralDeadState::Enter(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] DEAD Enter (Killed)\n");
}

void GeneralDeadState::Update(FSMComponent* fsm, float dt)
{
	// 죽은 상태는 별도의 전이 없이 서버의 리스폰 패킷을 기다림
}

void GeneralDeadState::Exit(FSMComponent* fsm)
{
	DEBUG_LOG_FMT("[FSM] DEAD Exit (Respawned)\n");
}
