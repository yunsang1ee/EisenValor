#include "stdafxClient.h"
#include "GeneralStates.h"
#include "FSMComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "AnimationComponent.h"
#include "ResourceGlobal.h"
#include "AnimationResource.h"
#include "SceneGlobal.h"
#include "CameraComponent.h"
#include <GameObject.h>
#include <GameObject.inl>
#include <Packets/Enums_generated.h>
#include <InputGlobal.h>
#include <Packets/C2SPackets.h>
#include <Component/FSM/FSMComponent.h>
#include "Util/GameConstants.h"

// ==================================
//		  GENERAL_IDLE_STATE
// ==================================
PlayerlIdleState::PlayerlIdleState(): State(FB_ENUMS::PLAYER_STATE_TYPE_IDLE)
{
}

void PlayerlIdleState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] IDLE Enter (Subject: {})\n", fsm->GetHandle().GetValue());

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), true);
		}
	}
}

void PlayerlIdleState::Update(FSMComponent* fsm, float dt)
{
}

void PlayerlIdleState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] IDLE Exit\n");
}

// ==================================
//		  GENERAL_MOVE_STATE
// ==================================
PlayerMoveState::PlayerMoveState(): State(FB_ENUMS::PLAYER_STATE_TYPE_MOVE)
{
}

void PlayerMoveState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] MOVE Enter (Subject: {})\n", fsm->GetHandle().GetValue());

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_MOVE), true);
		}
	}
}

void PlayerMoveState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	auto* obj = fsm->GetGameObject();
	if (!obj) return;

	auto* anim = obj->GetComponent<AnimationComponent>();
	if (!anim) return;

	// 기본 애니메이션 키 (전진:2)
	uint8_t targetKey = static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_MOVE);

	// 락온 상태일 경우 방향별 애니메이션 키 결정
	//if (fsm->IsLockOn())
	{
		auto dir = fsm->GetMoveDirection();
		switch (dir)
		{
		case FSMComponent::MoveDirection::BWD: targetKey = 21; break;
		case FSMComponent::MoveDirection::LFT: targetKey = 22; break;
		case FSMComponent::MoveDirection::RGT: targetKey = 23; break;
		default:                               targetKey = 2;  break;
		}
	}

	// 현재 재생 중인 애니메이션과 다를 때만 재생
	if (anim->GetCurrentKey() != targetKey)
	{
		anim->Play(targetKey, true);
	}
}

void PlayerMoveState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] MOVE Exit\n");
}

// ==================================
//		 GENERAL_PRE_DELAY_STATE
// ==================================
PlayerPreDelayState::PlayerPreDelayState() : State(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY)
{
}

void PlayerPreDelayState::Enter(FSMComponent* fsm)
{
	if (fsm)
	{
		fsm->SetStateTimer(0.0f);
		//DEBUG_LOG_FMT("[FSM] PRE_DELAY Enter\n");
	}
}

void PlayerPreDelayState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	fsm->AddStateTimer(dt);

	// FSM에 저장된 공격 타입을 사용 (버튼을 떼도 유지됨)
	GENERAL_ATTACK_TYPE type = static_cast<GENERAL_ATTACK_TYPE>(fsm->GetCurAttackType());

	// 약공격: 10FPS, 강공격: 20FPS
	float targetTime = (type == GENERAL_ATTACK_TYPE_HEAVY) ? (20.0f / 60.0f) : (10.0f / 60.0f);

	if (fsm->GetStateTimer() >= targetTime)
	{
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK);
	}
}

void PlayerPreDelayState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] PRE_DELAY Exit\n");
}

// ==================================
//		  GENERAL_ATTACK_STATE
// ==================================
PlayerAttackState::PlayerAttackState() : State(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK)
{
	SetHasExitTime(true);
	SetNextStateOnEnd(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY);
}

void PlayerAttackState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] ATTACK Enter!\n");
	fsm->SetStateTimer(0.0f);

	// 애니메이션 Key로 재생
	if (auto* go = fsm->GetGameObject())
	{
		if (auto* anim = go->GetComponent<AnimationComponent>())
		{
			// 공격 타입에 따라 다른 애니메이션 키 계산 (100번 구역 사용)
			uint8_t attackType = static_cast<uint8_t>(fsm->GetCurAttackType());
			uint8_t animationKey = 100 + attackType;
			anim->Play(animationKey, false, true);
		}
	}
}

void PlayerAttackState::Update(FSMComponent* fsm, float dt)
{
	// Onupdate에서 자동 체크
}

void PlayerAttackState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] ATTACK Exit\n");
}

// ==================================
//		 GENERAL_POST_DELAY_STATE
// ==================================
PlayerPostDelayState::PlayerPostDelayState() : State(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY)
{
}

void PlayerPostDelayState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] POST_DELAY Enter\n");
	fsm->SetStateTimer(0.0f);
}

void PlayerPostDelayState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	fsm->AddStateTimer(dt);

	// FSM에 저장된 공격 타입을 사용
	GENERAL_ATTACK_TYPE type = static_cast<GENERAL_ATTACK_TYPE>(fsm->GetCurAttackType());

	// 약공격: 5FPS, 강공격: 10FPS
	// ?
	float targetTime = (type == GENERAL_ATTACK_TYPE_HEAVY) ? (10.0f / 60.0f) : (5.0f / 60.0f);

	if (fsm->GetStateTimer() >= targetTime)
	{
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	}
}

void PlayerPostDelayState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] POST_DELAY Exit\n");
}

// ==================================
//		  GENERAL_DEFENSE_STATE
// ==================================
PlayerDefenseState::PlayerDefenseState() : State(FB_ENUMS::PLAYER_STATE_TYPE_DEFENSE)
{
}

void PlayerDefenseState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] DEFENSE Enter (Block Success!)\n");
	fsm->SetStateTimer(0.0f);
}

void PlayerDefenseState::Update(FSMComponent* fsm, float dt)
{
	fsm->AddStateTimer(dt);
	// 방어 성공 연출 시간 (1초)
	if (fsm->GetStateTimer() >= 1.0f)
	{
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	}
}

void PlayerDefenseState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] DEFENSE Exit\n");
}

// ==================================
//		  GENERAL_STUN_STATE
// ==================================
PlayerStunState::PlayerStunState() : State(FB_ENUMS::PLAYER_STATE_TYPE_STUN)
{
	SetHasExitTime(true);
	SetNextStateOnEnd(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
}

void PlayerStunState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] STUN Enter (Hit!)\n");
	fsm->SetStateTimer(0.0f);

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN), false, true);
		}
	}
}

void PlayerStunState::Update(FSMComponent* fsm, float dt)
{
	// OnUpdate에서 자동 체크
}

void PlayerStunState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] STUN Exit\n");
}

// ==================================
//		  GENERAL_DEAD_STATE
// ==================================
PlayerDeadState::PlayerDeadState() : State(FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
{
	SetHasExitTime(true);
}

void PlayerDeadState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] DEAD Enter (Killed)\n");

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD), false);
		}
	}
}

void PlayerDeadState::Update(FSMComponent* fsm, float dt)
{
	// DEAD는 전이 없이 서버의 리스폰 패킷을 기다림
}

void PlayerDeadState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] DEAD Exit (Respawned)\n");
}

// ==================================
//          SOLDIER_STATES
// ==================================

// Soldier Idle
SoldierIdleState::SoldierIdleState() : State(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE)
{
}

void SoldierIdleState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE), true);
		}
	}
}

void SoldierIdleState::Update(FSMComponent* fsm, float dt)
{
}

void SoldierIdleState::Exit(FSMComponent* fsm)
{
}

// Soldier Move
SoldierMoveState::SoldierMoveState() : State(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE)
{
}

void SoldierMoveState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE), true);
		}
	}
}

void SoldierMoveState::Update(FSMComponent* fsm, float dt)
{
}

void SoldierMoveState::Exit(FSMComponent* fsm)
{
}

//// Soldier Stun
//SoldierStunState::SoldierStunState() : State(FB_ENUMS::PLAYER_STATE_TYPE_STUN)
//{
//	SetHasExitTime(true);
//	SetNextStateOnEnd(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE));
//}
//
//void SoldierStunState::Enter(FSMComponent* fsm)
//{
//	if (auto* obj = fsm->GetGameObject())
//	{
//		if (auto* anim = obj->GetComponent<AnimationComponent>())
//		{
//			anim->Play(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN), false, true);
//		}
//	}
//}

//void SoldierStunState::Update(FSMComponent* fsm, float dt)
//{
//}
//
//void SoldierStunState::Exit(FSMComponent* fsm)
//{
//}

// Soldier Dead
SoldierDeadState::SoldierDeadState() : State(FB_ENUMS::SOLDIER_STATE_TYPE_DEAD)
{
	SetHasExitTime(true);
}

void SoldierDeadState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_DEAD), false);
		}
	}
}

void SoldierDeadState::Update(FSMComponent* fsm, float dt)
{
}

void SoldierDeadState::Exit(FSMComponent* fsm)
{
}

// Soldier Chase
SoldierChaseState::SoldierChaseState() : State(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE)
{
}

void SoldierChaseState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE), true);
		}
	}
}

void SoldierChaseState::Update(FSMComponent* fsm, float dt)
{
}

void SoldierChaseState::Exit(FSMComponent* fsm)
{
}

// Soldier Attack
SoldierAttackState::SoldierAttackState() : State(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK)
{
	SetHasExitTime(true);
	SetNextStateOnEnd(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE));
}

void SoldierAttackState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK), false, true);
		}
	}
}

void SoldierAttackState::Update(FSMComponent* fsm, float dt)
{
}

void SoldierAttackState::Exit(FSMComponent* fsm)
{
}
