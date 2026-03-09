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

Server::Contents::SoldierIdleState::SoldierIdleState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_IDLE }
{

}

Server::Contents::SoldierIdleState::~SoldierIdleState()
{
}

void Server::Contents::SoldierIdleState::Enter(const float dt)
{
}

void Server::Contents::SoldierIdleState::Exit(const float dt)
{
}

void Server::Contents::SoldierIdleState::Update(const float dt)
{
}

Server::Contents::SoldierMoveState::SoldierMoveState(const float viewRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_MOVE }, m_viewRangeSq{ viewRange * viewRange }, m_accDTForSearch{ 0.f }, m_currentWaypointIndex{}
{
	m_wayPoints.push_back(Vec3{ 20.f, 0.f, 0.f });
	m_wayPoints.push_back(Vec3{ 40.f, 0.f, 0.f });
	m_wayPoints.push_back(Vec3{ 60.f, 0.f, 0.f });
	m_wayPoints.push_back(Vec3{ 80.f, 0.f, 0.f });
	m_wayPoints.push_back(Vec3{ 100.f, 0.f, 0.f });
	m_wayPoints.push_back(Vec3{ 120.f, 0.f, 0.f });
	m_wayPoints.push_back(Vec3{ 140.f, 0.f, 0.f });
	m_wayPoints.push_back(Vec3{ 160.f, 0.f, 0.f });
}

Server::Contents::SoldierMoveState::~SoldierMoveState()
{
}

void Server::Contents::SoldierMoveState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierMoveState", id) << std::endl;

	if(m_currentWaypointIndex >= m_wayPoints.size()) {
		return;
	}

	const Vec3& dest = m_wayPoints[m_currentWaypointIndex];

	const auto ag{ owner->GetComponent<Server::Contents::NavAgent>() };
	if(ag) {
		owner->SetLook(dest);
		ag->SetDestPos(dest);
	}
}

void Server::Contents::SoldierMoveState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierMoveState", id) << std::endl;
}

void Server::Contents::SoldierMoveState::Update(const float dt)
{
	const auto owner = std::static_pointer_cast<Soldier>(GetFSM()->GetOwner());
	if(!IsValidObj(owner) || m_wayPoints.empty())
		return;

	const auto ag = owner->GetComponent<Server::Contents::NavAgent>();
	const auto& curPos = owner->GetPos();

	const Vec3& destPos = m_wayPoints[m_currentWaypointIndex];
	float distToDestSq = (destPos - curPos).LengthSquared();

	if(distToDestSq < 1.0f) {
		m_currentWaypointIndex++;
		std::cout << "Waypoint Index Increase!" << std::endl;
		if(m_currentWaypointIndex < m_wayPoints.size()) {
			if(ag) 
				ag->SetDestPos(m_wayPoints[m_currentWaypointIndex]);
		}
		else
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE, dt, true);
		return;
	}

	m_accDTForSearch += dt;

	if(m_accDTForSearch >= 0.1f) {
		m_accDTForSearch = 0.f;
		
		const auto world{ owner->GetGameWorld() };
		std::shared_ptr<Creature> target;

		float closestDistSq{ m_viewRangeSq };

		if(world) {
			const auto& groups{ world->GetGameObjectGroups() };
			
			for(const auto& group : groups) {
				for(const auto& [id, o] : group) {

					if(false == IsValidObj(o))
						continue;

					if(false == o->IsCreature())
						continue;

					std::shared_ptr<Creature> obj{ std::static_pointer_cast<Creature>(o) };

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
				GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
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
	std::cout << "Soldier Search State Enter" << std::endl;
}

void Server::Contents::SoldierSearchState::Exit(const float dt)
{
	std::cout << "Soldier Search State Exit" << std::endl;
}

void Server::Contents::SoldierSearchState::Update(const float dt)
{
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(false == IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
	}
	else {
		const auto distToTargetSq{ (target->GetPos() - owner->GetPos()).LengthSquared() };

		if(distToTargetSq < m_attackRangeSq)
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt, true);
		else {
			// 타겟이 추격 가능 시 추격
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
		}
	}
}


Server::Contents::SoldierChaseState::SoldierChaseState(const float chaseRange, const float attackRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_CHASE }, m_chaseRangeSq{ chaseRange * chaseRange }, m_attackRangeSq{attackRange * attackRange}
{
}

Server::Contents::SoldierChaseState::~SoldierChaseState()
{
}

void Server::Contents::SoldierChaseState::Enter(const float dt)
{
	std::cout << "SoldierChaseState Enter" << std::endl;
	
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };
}

void Server::Contents::SoldierChaseState::Exit(const float dt)
{
	std::cout << "SoldierChaseState Exit" << std::endl;
}

void Server::Contents::SoldierChaseState::Update(const float dt)
{
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(false == IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	if(target)
		owner->SetLook(target->GetPos());

	const float distToTargetSq{ (target->GetPos() - owner->GetPos()).LengthSquared() };

	// 추격 시, 공격 범위 안에 있으면 공격상태로 전환
	if(distToTargetSq < m_attackRangeSq) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt, true);
		return;
	}

	// 추격 시, 추격 범위를 벗어났다면 Move상태로 전환
	if(distToTargetSq > m_chaseRangeSq) {
		owner->SetTarget(nullptr);
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	// 추격
	const Vec3& targetPos{ target->GetPos() };
	const auto ag{ owner->GetComponent<Server::Contents::NavAgent>() };
	if(ag)
		ag->SetDestPos(targetPos);
}

Server::Contents::SoldierAttackState::SoldierAttackState(const float attackRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK }, m_accDTForAttack{ }, m_attackRangeSq{attackRange * attackRange}
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
	m_accDTForAttack = 0.f;
	const auto ag{ owner->GetComponent<Server::Contents::NavAgent>() };
	if(ag)
		ag->StopMove();
}

void Server::Contents::SoldierAttackState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierAttackState", id) << std::endl;
	m_accDTForAttack = 0.f;
}

void Server::Contents::SoldierAttackState::Update(const float dt)
{
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(false == IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	if(target)
		owner->SetLook(target->GetPos());

	if(target) {
		m_accDTForAttack += dt;

		if(m_accDTForAttack >= 1.f) {
			m_accDTForAttack = 0.f;
			const float distToTargetSq{ (target->GetPos() - owner->GetPos()).LengthSquared() };

			if(distToTargetSq < m_attackRangeSq) {
				if(target->OnDamaged(owner, dt)) {
					const auto& ownerPos{ owner->GetPos() };
					std::cout << ownerPos.x << ", " << ownerPos.y << ", " << ownerPos.z << std::endl;
					std::cout << "Soldier Attack Success!" << std::endl;
					m_accDTForAttack = 0.f;
				}
			}
			else {
				GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
			}
		}
	}
	else {
		m_accDTForAttack = 0.f;
		owner->SetTarget(nullptr);
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
	}
}

Server::Contents::SoldierDeadState::SoldierDeadState(const float deadAnimTime)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_DEAD }, m_accDT{}, m_deadAnimTime{ deadAnimTime }
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
	if(m_accDT >= m_deadAnimTime) {
		std::cout << "Soldier Dead!" << std::endl;
		const auto owner{ GetFSM()->GetOwner() };
		const auto world{ owner->GetGameWorld() };
		world->RemoveGameObject(owner);
	}
}

