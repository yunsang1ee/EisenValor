#include "stdafxClient.h"
#include "StatePool.h"
#include "StateImplementations.h"
#include <Packets/Enums_generated.h>
#include "Util/GameConstants.h"

// 정적 멤버 초기화
std::map<uint8_t, std::unique_ptr<State>> StatePool::s_instanceMap;

void StatePool::Initialize()
{
	// 이미 초기화된 경우 무시
	if (!s_instanceMap.empty()) return;

	// General States & Player States
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_IDLE] = std::make_unique<GeneralIdleState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_WALK] = std::make_unique<GeneralWalkState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_RUN] = std::make_unique<GeneralRunState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_ATTACK] = std::make_unique<GeneralAttackState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_STUN] = std::make_unique<GeneralStunState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_DEAD] = std::make_unique<GeneralDeadState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_DODGE] = std::make_unique<GeneralDodgeState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_ROLL] = std::make_unique<GeneralRollState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_GUARD] = std::make_unique<GeneralGuardState>();

	// 클라용
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY] = std::make_unique<GeneralPreDelayState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY] = std::make_unique<GeneralPostDelayState>();

	// Soldier States
	uint8_t offset = StateOffset::kSoldierOffset;
	s_instanceMap[offset + FB_ENUMS::SOLDIER_STATE_TYPE_IDLE] = std::make_unique<SoldierIdleState>();
	s_instanceMap[offset + FB_ENUMS::SOLDIER_STATE_TYPE_MOVE] = std::make_unique<SoldierMoveState>();
	//s_instanceMap[offset + FB_ENUMS::PLAYER_STATE_TYPE_STUN] = std::make_unique<SoldierStunState>();
	s_instanceMap[offset + FB_ENUMS::SOLDIER_STATE_TYPE_DEAD] = std::make_unique<SoldierDeadState>();
	s_instanceMap[offset + FB_ENUMS::SOLDIER_STATE_TYPE_CHASE] = std::make_unique<SoldierChaseState>();
	s_instanceMap[offset + FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK] = std::make_unique<SoldierAttackState>();

	// 상태 추가 시 여기에 등록
}

void StatePool::Release()
{
	s_instanceMap.clear();
}

State* StatePool::GetState(uint8_t type)
{
	auto iter = s_instanceMap.find(type);
	if (iter != s_instanceMap.end())
	{
		return iter->second.get();
	}
	return nullptr;
}
