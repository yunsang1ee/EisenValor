#include "stdafxClient.h"
#include "StateImplementations.h"
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

namespace
{
struct AttackTiming
{
	float preDelay;
	float attack;
	float postDelay;
};

AttackTiming GetAttackTiming(GENERAL_ATTACK_TYPE type)
{
	switch (type)
	{
	case GENERAL_ATTACK_TYPE_HEAVY:
		return {21.0f / 30.0f, 6.0f / 30.0f, 12.0f / 30.0f};
	case GENERAL_ATTACK_TYPE_AREA:
		return {30.0f / 30.0f, 6.0f / 30.0f, 21.0f / 30.0f};
	default:
		return {9.0f / 30.0f, 3.0f / 30.0f, 3.0f / 30.0f};
	}
}

uint8_t GetAttackAnimationKey(FSMComponent* fsm)
{
	const uint8_t dir = fsm ? fsm->GetCurAttackDir() : 0;
	const uint8_t attackType = fsm ? static_cast<uint8_t>(fsm->GetCurAttackType()) : 0;
	return static_cast<uint8_t>(100 + (dir * 10) + attackType);
}
}

// ==================================
//		  GENERAL_IDLE_STATE
// ==================================
GeneralIdleState::GeneralIdleState(): State(FB_ENUMS::PLAYER_STATE_TYPE_IDLE)
{
}

void GeneralIdleState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			if (fsm->GetStance() == static_cast<uint8_t>(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT))
			{
				uint8_t dir = fsm->GetCurAttackDir();
				uint8_t idleKey = StateOffset::kIdleOffset + dir; // 61:TOP, 62:LEFT, 63:RIGHT
				anim->PlayBlend(idleKey, AnimationOffset::kBlendDuration, true, true);
			}
			else
			{
				anim->PlayBlend(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), AnimationOffset::kBlendDuration, true, true);
			}
		}
	}
}

void GeneralIdleState::Update(FSMComponent* fsm, float dt)
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
		
		//DEBUG_LOG_FMT("[AnimDebug] FSM: {}, Obj: {}, Dir: {}, Key: {}\n", (void*)fsm, obj->GetName(), dir, targetIdleKey);

		if (anim->GetCurrentKey() != targetIdleKey)
		{
			anim->PlayBlend(targetIdleKey, AnimationOffset::kBlendDuration, true, true);
		}
	}
	else // 일반 태세일 때
	{
		uint8_t neutralIdleKey = static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
		if (anim->GetCurrentKey() != neutralIdleKey)
		{
			anim->Play(neutralIdleKey, true, true);
		}
	}
}

void GeneralIdleState::Exit(FSMComponent* fsm)
{	
}


// ==================================
// 		 GENERAL_WALK_STATE
// ==================================
GeneralWalkState::GeneralWalkState() : State(FB_ENUMS::PLAYER_STATE_TYPE_WALK)
{

}

void GeneralWalkState::Enter(FSMComponent* fsm) 
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

void GeneralWalkState::Update(FSMComponent* fsm, float dt)
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


	// 전투 자세(플레이어 락온 / NPC 장수 COMBAT 스탠스)일 때 방향별 애니메이션 키 결정
	const bool isCombat = fsm->IsLockOn() ||
		fsm->GetStance() == static_cast<uint8_t>(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT);
	if (isCombat)
	{
		auto dir = fsm->GetMoveDirection();
		switch (dir)
		{
		case FB_ENUMS::MOVE_DIRECTION_TYPE_FWD:
			targetKey = 20;
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

void GeneralWalkState::Exit(FSMComponent* fsm)
{

}


// ==================================
// 		 GENERAL_RUN_STATE
// ==================================
GeneralRunState::GeneralRunState() :State(FB_ENUMS::PLAYER_STATE_TYPE_RUN)
{

}

void GeneralRunState::Enter(FSMComponent* fsm)
{
	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_RUN), true);
		}
	}
}

void GeneralRunState::Update(FSMComponent* fsm, float dt)
{
}

void GeneralRunState::Exit(FSMComponent* fsm)
{

}

// Dodge
// ==================================
//		 GENERAL_DODGE_STATE
// ==================================
GeneralDodgeState::GeneralDodgeState() : State(FB_ENUMS::PLAYER_STATE_TYPE_DODGE)
{
	SetHasExitTime(true);
	SetNextStateOnEnd(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
}

void GeneralDodgeState::Enter(FSMComponent* fsm)
{
	if (!fsm) return;
	//DEBUG_LOG_FMT("[FSM] DODGE Enter\n");

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			uint8_t dodgeKey = 200;
			auto dir = fsm->GetDodgeDirection();
			switch (dir)
			{
			case FB_ENUMS::MOVE_DIRECTION_TYPE_BWD:
				dodgeKey = 201;
				break;
			case FB_ENUMS::MOVE_DIRECTION_TYPE_LFT:
				dodgeKey = 202;
				break;
			case FB_ENUMS::MOVE_DIRECTION_TYPE_RGT:
				dodgeKey = 203;
				break;
			case FB_ENUMS::MOVE_DIRECTION_TYPE_FWD:
			default:
				dodgeKey = 200;
				break;
			}
			anim->Play(dodgeKey, false, true);
		}
	}
}

void GeneralDodgeState::Update(FSMComponent* fsm, float dt)
{
}

void GeneralDodgeState::Exit(FSMComponent* fsm)
{
}

// Roll
// ==================================
//		 GENERAL_ROLL_STATE
// ==================================
GeneralRollState::GeneralRollState() : State(FB_ENUMS::PLAYER_STATE_TYPE_ROLL)
{
	SetHasExitTime(true);
	SetNextStateOnEnd(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
}

void GeneralRollState::Enter(FSMComponent* fsm)
{
	if (!fsm) return;
	//DEBUG_LOG_FMT("[FSM] ROLL Enter\n");

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			anim->Play(204, false, true);
		}
	}
}

void GeneralRollState::Update(FSMComponent* fsm, float dt)
{
}

void GeneralRollState::Exit(FSMComponent* fsm)
{
}


// ==================================
//		 GENERAL_PRE_DELAY_STATE
// ==================================
GeneralPreDelayState::GeneralPreDelayState() : State(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY)
{
}

void GeneralPreDelayState::Enter(FSMComponent* fsm)
{
	if (fsm)
	{
		fsm->SetStateTimer(0.0f);
		if (auto* go = fsm->GetGameObject())
		{
			if (auto* anim = go->GetComponent<AnimationComponent>())
			{
				anim->Play(GetAttackAnimationKey(fsm), false, true);
			}
		}
		//DEBUG_LOG_FMT("[FSM] PRE_DELAY Enter\n");
	}
}

void GeneralPreDelayState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	fsm->AddStateTimer(dt);

	// FSM에 저장된 공격 타입을 사용 (버튼을 떼도 유지됨)
	GENERAL_ATTACK_TYPE type = static_cast<GENERAL_ATTACK_TYPE>(fsm->GetCurAttackType());

	// 약공격: 10FPS, 강공격: 20FPS
	const float targetTime = GetAttackTiming(type).preDelay;

	if (fsm->GetStateTimer() >= targetTime)
	{
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK);
	}
}

void GeneralPreDelayState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] PRE_DELAY Exit\n");
}

// ==================================
//		  GENERAL_ATTACK_STATE
// ==================================
GeneralAttackState::GeneralAttackState() : State(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK)
{
	SetHasExitTime(false);
}

void GeneralAttackState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] ATTACK Enter!\n");
	fsm->SetStateTimer(0.0f);
	return;

	/*DEBUG_LOG_FMT("\n[FSM] Playing Attack Animation - Type: {}, Dir: {}, LockOn: {}\n", 
		fsm->GetCurAttackType(),
		fsm->GetCurAttackDir(), fsm->IsLockOn()
	);*/

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

void GeneralAttackState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	fsm->AddStateTimer(dt);

	GENERAL_ATTACK_TYPE type = static_cast<GENERAL_ATTACK_TYPE>(fsm->GetCurAttackType());
	const float targetTime = GetAttackTiming(type).attack;

	if (fsm->GetStateTimer() >= targetTime)
	{
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY);
		return;
	}

	// Onupdate에서 자동 체크
}

void GeneralAttackState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] ATTACK Exit\n");
}

// ==================================
//		 GENERAL_POST_DELAY_STATE
// ==================================
GeneralPostDelayState::GeneralPostDelayState() : State(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY)
{
}

void GeneralPostDelayState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] POST_DELAY Enter\n");
	fsm->SetStateTimer(0.0f);
}

void GeneralPostDelayState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	fsm->AddStateTimer(dt);

	// FSM에 저장된 공격 타입 사용
	GENERAL_ATTACK_TYPE type = static_cast<GENERAL_ATTACK_TYPE>(fsm->GetCurAttackType());

	// 약공격: 5FPS, 강공격: 10FPS
	// ?
	const float targetTime = GetAttackTiming(type).postDelay;

	if (fsm->GetStateTimer() >= targetTime)
	{
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	}
}

void GeneralPostDelayState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] POST_DELAY Exit\n");
}

// ==================================
//		  GENERAL_STUN_STATE
// ==================================
GeneralStunState::GeneralStunState() : State(FB_ENUMS::PLAYER_STATE_TYPE_STUN)
{
	SetHasExitTime(true);
	SetNextStateOnEnd(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
}

void GeneralStunState::Enter(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] STUN Enter (Hit!)\n");
	fsm->SetStateTimer(0.0f);

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			//// 피격 방향(1, 2, 3) & 공격 강도(0:Light, 1:Heavy) 반영
			// 기본 STUN
			uint8_t dir = fsm->GetCurAttackDir();
			uint8_t type = static_cast<uint8_t>(fsm->GetCurAttackType());

			uint8_t stunKey = StateOffset::kHurtOffset + static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN);
			if (dir >= 1 && dir <= 4)
			{
				stunKey = 150 + (dir * 10) + std::min<uint8_t>(type, 1);
			}

			DEBUG_LOG_FMT(
				"[HitReact] dir={}, attackType={}, reactType={}, stunKey={}\n",
				static_cast<int>(dir),
				static_cast<int>(type),
				static_cast<int>(std::min<uint8_t>(type, 1)),
				static_cast<int>(stunKey)
			);

			anim->Play(stunKey, false, true);
		}
	}
}

void GeneralStunState::Update(FSMComponent* fsm, float dt)
{
	// OnUpdate에서 자동 체크
}

void GeneralStunState::Exit(FSMComponent* fsm)
{
	//DEBUG_LOG_FMT("[FSM] STUN Exit\n");
}

// ==================================
//		  GENERAL_GUARD_STATE
// ==================================
GeneralGuardState::GeneralGuardState() : State(FB_ENUMS::PLAYER_STATE_TYPE_GUARD)
{
	SetHasExitTime(false);
}

void GeneralGuardState::Enter(FSMComponent* fsm)
{
	if (!fsm) return;

	fsm->SetStateTimer(0.0f);

	if (auto* obj = fsm->GetGameObject())
	{
		if (auto* anim = obj->GetComponent<AnimationComponent>())
		{
			switch (fsm->GetGuardRole())
			{
			case FSMComponent::GuardRole::Defender:
				anim->Play(210, false, true);
				break;
			case FSMComponent::GuardRole::Attacker:
				anim->Play(161, false, true);
				break;
			default:
				anim->Play(210, false, true);
				break;
			}
		}
	}
}

void GeneralGuardState::Update(FSMComponent* fsm, float dt)
{
	if (!fsm) return;

	auto* obj = fsm->GetGameObject();
	if (!obj) return;

	auto* anim = obj->GetComponent<AnimationComponent>();
	if (!anim) return;

	if (fsm->GetGuardRole() == FSMComponent::GuardRole::Attacker)
	{
		if (anim->IsAnimationEnd())
		{
			fsm->SetGuardRole(FSMComponent::GuardRole::None);
			fsm->RequestState(FSMComponent::StateRequestType::IdleRecovery);
		}
		return;
	}

	if (anim->IsAnimationEnd())
	{
		fsm->SetGuardRole(FSMComponent::GuardRole::None);
		fsm->RequestState(FSMComponent::StateRequestType::IdleRecovery);
	}
}

void GeneralGuardState::Exit(FSMComponent* fsm)
{
	if (!fsm) return;
	fsm->SetGuardRole(FSMComponent::GuardRole::None);
}

// ==================================
//		  GENERAL_DEAD_STATE
// ==================================
GeneralDeadState::GeneralDeadState() : State(FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
{
	SetHasExitTime(true);
}

void GeneralDeadState::Enter(FSMComponent* fsm)
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

void GeneralDeadState::Update(FSMComponent* fsm, float dt)
{
	// DEAD는 전이 없이 서버의 리스폰 패킷을 기다림
}

void GeneralDeadState::Exit(FSMComponent* fsm)
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
	SetNextStateOnEnd(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE));
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
