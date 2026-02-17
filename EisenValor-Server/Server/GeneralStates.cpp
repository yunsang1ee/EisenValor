#include "pch.h"
#include "GeneralStates.h"

#include "GameObject.h"
#include "BehaviorTree.h"

#include "GameWorld.h"

Server::Contents::GeneralRoamingState::GeneralRoamingState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING)
{
	std::cout << "General GeneralRoamingState Enter!" << std::endl;
	// TODO: 행동트리 생성
}

Server::Contents::GeneralRoamingState::~GeneralRoamingState()
{
}

void Server::Contents::GeneralRoamingState::Enter(const float dt)
{
}

void Server::Contents::GeneralRoamingState::Exit(const float dt)
{
	
}

void Server::Contents::GeneralRoamingState::Update(const float dt)
{
}

Server::Contents::GeneralDuelingState::GeneralDuelingState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_DUELING)
{
}

Server::Contents::GeneralDuelingState::~GeneralDuelingState()
{
}

void Server::Contents::GeneralDuelingState::Enter(const float dt)
{
	std::cout << "General GeneralDuelingState Enter!" << std::endl;
	// TODO: 행동트리 생성
}

void Server::Contents::GeneralDuelingState::Exit(const float dt)
{

}

void Server::Contents::GeneralDuelingState::Update(const float dt)
{
/*
	- 상대가 공격 O
		- 약공격 방어 확률 30%
		- 강공격 방어 확률 90%
		- 반격 확률 20%
	- 상대가 공격 X
		- 공격할 확률 60% (이때 스탠스를 바꿀 확률 50%)
		- 약공격 확률 40%
			- 약공격으로 시작하는 콤보 중 하나 실행확률 30%
		- 강공격 확률 50%
			- 페이크확률 60%
			- 강공격으로 시작하는 콤보 중 하나 실행확률 40%
		- 방어해제공격 10%
	- 움직이거나 스탠스만 변경할 확률 40%
*/
}

Server::Contents::GeneralDeadState::GeneralDeadState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_DEAD)
{
}

Server::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void Server::Contents::GeneralDeadState::Enter(const float dt)
{
	std::cout << "General GeneralDuelingState Enter!" << std::endl;
	// TODO: 행동트리 생성
}

void Server::Contents::GeneralDeadState::Exit(const float dt)
{
}

void Server::Contents::GeneralDeadState::Update(const float dt)
{
}
