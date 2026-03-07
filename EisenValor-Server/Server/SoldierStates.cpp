#include "pch.h"
#include "SoldierStates.h"

#include "GameRoom.h"
#include "Player.h"
#include "FSM.h"
#include "NavAgent.h"
#include "GameWorld.h"
#include "Soldier.h"

Server::Contents::SoldierSpawnState::SoldierSpawnState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_SPAWN }, m_accDT{}
{
}

Server::Contents::SoldierSpawnState::~SoldierSpawnState()
{
}

void Server::Contents::SoldierSpawnState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter Soldier Idle", id) << std::endl;
	m_accDT = 0.f;
}

void Server::Contents::SoldierSpawnState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit Soldier Idle", id) << std::endl;
	m_accDT = 0.f;
}

void Server::Contents::SoldierSpawnState::Update(const float dt)
{
	m_accDT += dt;

	if(m_accDT >= 2.f) {
		auto const fsm = GetFSM();
		fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
	}
}

Server::Contents::SoldierMoveState::SoldierMoveState(const float viewRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_MOVE }, m_viewRangeSq{viewRange * viewRange}, m_accDTForSearch{0.f}
{
	m_wayPoints.push(Vec3{ 20.f, 0.f, 0.f });
	m_wayPoints.push(Vec3{ 40.f, 0.f, 0.f });
	m_wayPoints.push(Vec3{ 60.f, 0.f, 0.f });
	m_wayPoints.push(Vec3{ 80.f, 0.f, 0.f });
	m_wayPoints.push(Vec3{ 100.f, 0.f, 0.f });
	m_wayPoints.push(Vec3{ 120.f, 0.f, 0.f });
	m_wayPoints.push(Vec3{ 140.f, 0.f, 0.f });
	m_wayPoints.push(Vec3{ 160.f, 0.f, 0.f });
}

Server::Contents::SoldierMoveState::~SoldierMoveState()
{
}

void Server::Contents::SoldierMoveState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierMoveState", id) << std::endl;

	if(m_wayPoints.empty())
		return;

	const auto dest{ m_wayPoints.front() };

	const auto ag{ owner->GetComponent<Server::Contents::NavAgent>() };
	if(ag)
		ag->SetDestPos(dest);
}

void Server::Contents::SoldierMoveState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierMoveState", id) << std::endl;
}

void Server::Contents::SoldierMoveState::Update(const float dt)
{
	// TODO: SoldierMoveState Update
	const auto owner{ static_cast<Server::Contents::Soldier*>(GetFSM()->GetOwner()) };

	if(false == IsValidObj(owner))
		return;

	if(m_wayPoints.empty())
		return;

	const auto destPos{ m_wayPoints.front() };
	const auto& curPos{ owner->GetPos() };

	const float distToDestSq{ (destPos - curPos).LengthSquared() };

	if(distToDestSq < 0.01f) {
		m_wayPoints.pop();
		const Vec3& nextDest{ m_wayPoints.front() };
		const auto ag{ owner->GetComponent<Server::Contents::NavAgent>() };
		if(ag)
			ag->SetDestPos(nextDest);
	}

	m_accDTForSearch += dt;

	if(m_accDTForSearch >= 0.1f) {
		m_accDTForSearch = 0.f;
		
		const auto world{ owner->GetGameWorld() };
		Creature* target{ nullptr };

		float closestDistSq{ m_viewRangeSq };

		if(world) {
			const auto& groups{ world->GetGameObjectGroups() };
			
			for(const auto& group : groups) {
				for(const auto& [id, o] : group) {

					if(false == IsValidObj(o.get()))
						continue;

					Creature* obj{ static_cast<Creature*>(o.get()) };

					if(id == owner->GetID())
						continue;

					if(owner->GetTeamType() == obj->GetTeamType())
						continue;


					const Vec3 diff{ obj->GetPos() - owner->GetPos() };
					const float distToTargetSq{ diff.LengthSquared() };

					if(distToTargetSq < closestDistSq) {
						closestDistSq = distToTargetSq;
						target = obj;
					}
				}

				if(target)
					break;
			}

			if(target) {
				std::cout << "Soldier Move To Search" << std::endl;
				GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_SEARCH, dt, true);
				owner->SetTarget(target);
			}
			else
				owner->SetTarget(nullptr);
		}
	}
}

Server::Contents::SoldierSearchState::SoldierSearchState(const float attackRange)
	:State{FB_ENUMS::SOLDIER_STATE_TYPE_SEARCH}, m_attackRangeSq{attackRange * attackRange}
{

}

Server::Contents::SoldierSearchState::~SoldierSearchState()
{
}

void Server::Contents::SoldierSearchState::Enter(const float dt)
{
}

void Server::Contents::SoldierSearchState::Exit(const float dt)
{
}

void Server::Contents::SoldierSearchState::Update(const float dt)
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
	// TODO: SoldierChaseState Update
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
	// TODO: SoldierAttackState Update
}

Server::Contents::SoldierDeadState::SoldierDeadState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_DEAD }, m_accDT{}
{

}

Server::Contents::SoldierDeadState::~SoldierDeadState()
{
}

void Server::Contents::SoldierDeadState::Enter(const float dt)
{
	m_accDT = 0.f;
}

void Server::Contents::SoldierDeadState::Exit(const float dt)
{
	m_accDT = 0.f;
}

void Server::Contents::SoldierDeadState::Update(const float dt)
{
	m_accDT += dt;
	
	// TODO: 죽는 애니메이션 시간동안 대기
	if(m_accDT >= 3.f) {
		std::cout << "Soldier Dead!" << std::endl;
		const auto owner{ GetFSM()->GetOwner() };
		const auto world{ owner->GetGameWorld() };
		world->RemoveGameObject(owner);
	}
}
