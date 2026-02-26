#include "pch.h"
#include "SoldierStates.h"

#include "GameRoom.h"
#include "Player.h"
#include "FSM.h"
#include "NavAgent.h"
#include "GameWorld.h"

Server::Contents::SoldierIdleState::SoldierIdleState(const float enemyDetectionRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_IDLE }, m_enemyDetectionRangeSq{enemyDetectionRange * enemyDetectionRange }
{
}

Server::Contents::SoldierIdleState::~SoldierIdleState()
{
}

void Server::Contents::SoldierIdleState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter Soldier Idle", id) << std::endl;
}

void Server::Contents::SoldierIdleState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit Soldier Idle", id) << std::endl;
}

void Server::Contents::SoldierIdleState::Update(const float dt)
{
	auto const fsm = GetFSM();
	const auto owner = fsm->GetOwner();
	Vec3 curPos = owner->GetPos();
	curPos.x += 10.f * dt;
	if(auto navAgent = owner->GetComponent<Server::Contents::NavAgent>()) {
		navAgent->SetDestPos(curPos);

		auto pb{ ServerPackets::Make_SC_MOVE_PACKET(owner->GetID(), owner->GetPosInfo(), fsm->GetCurState()->GetStateType(), 0) };
		owner->GetGameWorld()->Broadcast(std::move(pb));
	}
}

Server::Contents::SoldierMoveState::SoldierMoveState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_MOVE }
{
}

Server::Contents::SoldierMoveState::~SoldierMoveState()
{
}

void Server::Contents::SoldierMoveState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierRunState", id) << std::endl;
}

void Server::Contents::SoldierMoveState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierRunState", id) << std::endl;
}

void Server::Contents::SoldierMoveState::Update(const float dt)
{
}

Server::Contents::SoldierChaseState::SoldierChaseState(const float chaseSpeed, const float combatRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_CHASE }, m_chaseSpeed{chaseSpeed}, m_combatRange{combatRange}
{
}

Server::Contents::SoldierChaseState::~SoldierChaseState()
{
}

void Server::Contents::SoldierChaseState::Enter(const float dt)
{

}

void Server::Contents::SoldierChaseState::Exit(const float dt)
{

}

void Server::Contents::SoldierChaseState::Update(const float dt)
{
}

Server::Contents::SoldierAttackState::SoldierAttackState(const float combatRange, const std::chrono::seconds attackCycleTime)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK }, m_accDt{ 0.f }, m_combatRange{combatRange}, m_attackCycleTime{attackCycleTime}
{
}

Server::Contents::SoldierAttackState::~SoldierAttackState()
{
}

void Server::Contents::SoldierAttackState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierAttackState", id) << std::endl;
}

void Server::Contents::SoldierAttackState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierAttackState", id) << std::endl;
	m_accDt = 0.f;
}

void Server::Contents::SoldierAttackState::Update(const float dt)
{
}

Server::Contents::SoldierDefenseState::SoldierDefenseState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE }, m_accDT{0.f}
{

}

Server::Contents::SoldierDefenseState::~SoldierDefenseState()
{

}

void Server::Contents::SoldierDefenseState::Enter(const float dt)
{

}

void Server::Contents::SoldierDefenseState::Exit(const float dt)
{
	m_accDT = 0.f;
}

void Server::Contents::SoldierDefenseState::Update(const float dt)
{
}

Server::Contents::SoldierStunState::SoldierStunState(const float stunTime)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_STUN }, m_stunTime{ stunTime }, m_accForStun{ 0.f }
{
}

Server::Contents::SoldierStunState::~SoldierStunState()
{
}

void Server::Contents::SoldierStunState::Enter(const float dt)
{

}

void Server::Contents::SoldierStunState::Exit(const float dt)
{
	m_accForStun = 0.f;
}

void Server::Contents::SoldierStunState::Update(const float dt)
{
}
