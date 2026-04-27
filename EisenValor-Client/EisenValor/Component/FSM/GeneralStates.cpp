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

using namespace DirectX;

// ==================================
//		  PLAYER_IDLE_STATE
// ==================================
PlayerlIdleState::PlayerlIdleState(): State(FB_ENUMS::PLAYER_STATE_TYPE_IDLE)
{
}

void PlayerlIdleState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			if (fsm->GetStance() == static_cast<uint8_t>(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT))
			{
				uint8_t dir = fsm->GetCurAttackDir();
				uint8_t idleKey = 60 + dir; // 61:TOP, 62:LEFT, 63:RIGHT
				anim->Play(idleKey, true);
			}
			else
			{
				anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), true);
			}
		}
	}
}

void PlayerlIdleState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	auto* obj = fsm->GetGameObject();
	if (!obj) return;

	auto* anim = obj->GetComponent<AnimationComponent>();
	if (!anim) return;

	// 전투 자세일 때만 방향별 실시간 Idle 교체 수행
	if (fsm->GetStance() == static_cast<uint8_t>(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT))
	{
		uint8_t dir = fsm->GetCurAttackDir();
		uint8_t targetIdleKey = StateOffset::kIdleOffset + dir;

		 if (anim->GetCurrentKey() != targetIdleKey)
		{
			//DEBUG_LOG_FMT("[Anim] Switching Idle Animation to Key: {} (Dir: {})\n", targetIdleKey, dir);
			anim->Play(targetIdleKey, true);
		}
	}
	else // 일반 태세일 때
	{
		uint8_t neutralIdleKey = static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
		if (anim->GetCurrentKey() != neutralIdleKey)
		{
			anim->Play(neutralIdleKey, true);
		}
	}
}

void PlayerlIdleState::Exit(FSMComponent* fsm)
{	
}


// ==================================
// 		 PLAYER_WALK_STATE
// ==================================
PlayerWalkState::PlayerWalkState() : State(FB_ENUMS::PLAYER_STATE_TYPE_WALK)
{

}

void PlayerWalkState::Enter(FSMComponent* fsm) 
{
	// DEBUG_LOG_FMT("[FSM] MOVE Enter (Subject: {})\n", fsm->GetHandle().GetValue());

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_WALK), true);
		}
	}
}

void PlayerWalkState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm)
		return;

	auto* obj = fsm->GetGameObject();
	if (!obj)
		return;

	auto* anim = obj->GetComponent<AnimationComponent>();
	if (!anim)
		return;

	// 기본 애니메이션 키 (전진:2)
	uint8_t targetKey = static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_WALK);


	// 락온 상태일 경우 방향별 애니메이션 키 결정
	if (fsm->IsLockOn())
	{
		auto dir = fsm->GetMoveDirection();
		switch (dir)
		{
		case FB_ENUMS::MOVE_DIRECTION_TYPE_FWD:
			targetKey = static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_WALK);
			break;
		case FB_ENUMS::MOVE_DIRECTION_TYPE_BWD:
			targetKey = 21;
			break;
		case FB_ENUMS::MOVE_DIRECTION_TYPE_LFT:
			targetKey = 22;
			break;
		case FB_ENUMS::MOVE_DIRECTION_TYPE_RGT:
			targetKey = 23;
			break;
		}
	}

	// 현재 재생 중인 애니메이션과 다를 때만 재생
	if (anim->GetCurrentKey() != targetKey)
	{
		anim->Play(targetKey, true);
	}
}

void PlayerWalkState::Exit(FSMComponent* fsm)
{

}


// ==================================
// 		 PLAYER_RUN_STATE
// ==================================
PlayerRunState::PlayerRunState() :State(FB_ENUMS::PLAYER_STATE_TYPE_RUN)
{

}

void PlayerRunState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_RUN), true);
		}
	}
}

void PlayerRunState::Update(FSMComponent* fsm, float dt)
{
}

void PlayerRunState::Exit(FSMComponent* fsm)
{

}


// ==================================
//		 PLAYER_PRE_DELAY_STATE
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
//		  PLAYER_ATTACK_STATE
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

	DEBUG_LOG_FMT("\n[FSM] Playing Attack Animation - Type: {}, Dir: {}, LockOn: {}\n", 
		fsm->GetCurAttackType(),
		fsm->GetCurAttackDir(), fsm->IsLockOn()
	);

	// 애니메이션 Key로 재생
	if (auto* go = fsm->GetGameObject())
	{
		if (auto* anim = go->GetComponent<AnimationComponent>())
		{
			// 공격 조준 방향(10단위) + 공격 타입(1단위) 조합 
			uint8_t dir = fsm->GetCurAttackDir();
			uint8_t attackType = static_cast<uint8_t>(fsm->GetCurAttackType());

			// 110(TOP), 120(LEFT), 130(RIGHT) 기반 + type
			uint8_t animationKey = 100 + (dir * 10) + attackType;
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
//		 PLAYER_POST_DELAY_STATE
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

	// FSM에 저장된 공격 타입 사용
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
//		  PLAYER_DEFENSE_STATE
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
//		  PLAYER_STUN_STATE
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
			//// 피격 방향(1, 2, 3) & 공격 강도(0:Light, 1:Heavy) 반영
			//uint8_t dir = fsm->GetCurAttackDir();
			//uint8_t type = static_cast<uint8_t>(fsm->GetCurAttackType()); // 0 or 1
			//
			//uint8_t stunKey = 150 + (dir * 10) + type;
			//
			//anim->Play(stunKey, false, true);

			// 기본 STUN
			anim->Play(StateOffset::kHurtOffset + static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN), false, true);
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
//		  PLAYER_DEAD_STATE
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
{}