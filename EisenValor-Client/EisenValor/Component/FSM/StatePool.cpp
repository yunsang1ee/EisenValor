#include "stdafxClient.h"
#include "StatePool.h"
#include "GeneralStates.h"
#include <Packets/Enums_generated.h>

// 정적 멤버 초기화
std::map<uint8_t, std::unique_ptr<State>> StatePool::s_instanceMap;

void StatePool::Initialize()
{
	// 이미 초기화된 경우 무시
	if (!s_instanceMap.empty()) return;

	// General States 
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_IDLE] = std::make_unique<PlayerlIdleState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_MOVE] = std::make_unique<PlayerMoveState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_STUN] = std::make_unique<PlayerStunState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_DEAD] = std::make_unique<PlayerDeadState>();

	// 클라용
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY] = std::make_unique<PlayerPreDelayState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_ATTACK] = std::make_unique<PlayerAttackState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY] = std::make_unique<PlayerPostDelayState>();
	s_instanceMap[FB_ENUMS::PLAYER_STATE_TYPE_DEFENSE] = std::make_unique<PlayerDefenseState>();

	// Soldier States
	s_instanceMap[50 + FB_ENUMS::SOLDIER_STATE_TYPE_IDLE] = std::make_unique<SoldierIdleState>();
	s_instanceMap[50 + FB_ENUMS::SOLDIER_STATE_TYPE_MOVE] = std::make_unique<SoldierMoveState>();
	s_instanceMap[50 + FB_ENUMS::PLAYER_STATE_TYPE_STUN] = std::make_unique<SoldierStunState>();
	s_instanceMap[50 + FB_ENUMS::SOLDIER_STATE_TYPE_DEAD] = std::make_unique<SoldierDeadState>();

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
