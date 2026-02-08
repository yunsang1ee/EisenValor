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

	// General States 등록
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_IDLE] = std::make_unique<GeneralIdleState>();
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_MOVE] = std::make_unique<GeneralMoveState>();
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_PRE_DELAY] = std::make_unique<GeneralPreDelayState>();
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_ATTACK] = std::make_unique<GeneralAttackState>();
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_POST_DELAY] = std::make_unique<GeneralPostDelayState>();
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_DEFENSE] = std::make_unique<GeneralDefenseState>();
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_STUN] = std::make_unique<GeneralStunState>();
	s_instanceMap[FB_ENUMS::GENERAL_STATE_TYPE_DEAD] = std::make_unique<GeneralDeadState>();
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
