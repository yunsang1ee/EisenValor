#include "pch.h"
#include "SoldierStates.h"

#include "Player.h"
#include "FSM.h"
#include "NavAgent.h"
#include "GameWorld.h"
#include "Soldier.h"

#define PRINT_SOLDIER_STATE_LOG

GameServer::Contents::SoldierSpawnState::SoldierSpawnState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_SPAWN }, m_accDT{}
{
}

GameServer::Contents::SoldierSpawnState::~SoldierSpawnState()
{
}

void GameServer::Contents::SoldierSpawnState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Enter Soldier Spawn", id) << std::endl;
#endif
	m_accDT = 0.f;
}

void GameServer::Contents::SoldierSpawnState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Exit Soldier Spawn", id) << std::endl;
#endif

	m_accDT = 0.f;
}

void GameServer::Contents::SoldierSpawnState::Update(const float dt)
{
	m_accDT += dt;

	if(m_accDT >= SPAWN_DELAY) {
		auto const fsm = GetFSM();
		fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
	}
}

GameServer::Contents::SoldierIdleState::SoldierIdleState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_IDLE }
{

}

GameServer::Contents::SoldierIdleState::~SoldierIdleState()
{
}

void GameServer::Contents::SoldierIdleState::Enter(const float dt)
{
}

void GameServer::Contents::SoldierIdleState::Exit(const float dt)
{
}

void GameServer::Contents::SoldierIdleState::Update(const float dt)
{
}

GameServer::Contents::SoldierMoveState::SoldierMoveState(const float viewRange, const std::vector<Vec3>& wayPoints)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_MOVE }, m_viewRangeSq{ viewRange * viewRange }, m_accDTForSearch{ 0.f }, m_currentWaypointIndex{}
{
	m_wayPoints = wayPoints;
}

GameServer::Contents::SoldierMoveState::~SoldierMoveState()
{
}

void GameServer::Contents::SoldierMoveState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Enter SoldierMoveState", id) << std::endl;
#endif

	if(m_currentWaypointIndex >= m_wayPoints.size()) {
		return;
	}

	const Vec3& dest = m_wayPoints[m_currentWaypointIndex];

	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(ag) {
		owner->LookAt(dest);
		ag->SetDestPos(dest);
	}
}

void GameServer::Contents::SoldierMoveState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Exit SoldierMoveState", id) << std::endl;
#endif
}

void GameServer::Contents::SoldierMoveState::Update(const float dt)
{
	const auto owner = std::static_pointer_cast<Soldier>(GetFSM()->GetOwner());
	if(!IsValidObj(owner) || m_wayPoints.empty())
		return;

	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	const Vec3& destPos = m_wayPoints[m_currentWaypointIndex];
	const float distToDestSq = (destPos - owner->GetPosition()).LengthSquared();

	if(distToDestSq < 1.0f) {
		m_currentWaypointIndex++;
#ifdef PRINT_SOLDIER_STATE_LOG
		std::cout << "Waypoint Index Increase!" << std::endl;
#endif

		if(m_currentWaypointIndex < m_wayPoints.size()) {
			if(ag)
				ag->SetDestPos(m_wayPoints[m_currentWaypointIndex]);
		}
		else {
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE, dt, true);
		}
		return;
	}

	m_accDTForSearch += dt;
	if(m_accDTForSearch < SEARCH_INTERVAL)
		return;

	m_accDTForSearch = 0.f;

	const auto world{ owner->GetGameWorld() };
	if(!world)
		return;

	std::shared_ptr<Creature> target;
	float closestDistSq{ m_viewRangeSq };

	for(const auto& group : world->GetGameObjectGroups()) {
		for(const auto& [id, o] : group) {
			if(!IsValidObj(o) || !o->IsCreature())
				continue;

			auto obj{ std::static_pointer_cast<Creature>(o) };

			if(id == owner->GetID() || owner->GetTeamType() == obj->GetTeamType())
				continue;

			const float distSq{ (obj->GetPosition() - owner->GetPosition()).LengthSquared() };
			if(distSq < closestDistSq) {
				closestDistSq = distSq;
				target = obj;
			}
		}
	}

	if(target) {
		owner->SetTarget(target);
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
	}
	else {
		owner->SetTarget(nullptr);
	}
}

GameServer::Contents::SoldierSearchState::SoldierSearchState(const float attackRange)
	:State{FB_ENUMS::SOLDIER_STATE_TYPE_SEARCH}, m_attackRangeSq{attackRange * attackRange}
{

}

GameServer::Contents::SoldierSearchState::~SoldierSearchState()
{
}

void GameServer::Contents::SoldierSearchState::Enter(const float dt)
{
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Enter Soldier Search State", GetFSM()->GetOwner()->GetID()) << std::endl;
#endif
}

void GameServer::Contents::SoldierSearchState::Exit(const float dt)
{
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Exit Soldier Search State", GetFSM()->GetOwner()->GetID()) << std::endl;
#endif
}

void GameServer::Contents::SoldierSearchState::Update(const float dt)
{
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(false == IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
	}
	else {
		const auto distToTargetSq{ (target->GetPosition() - owner->GetPosition()).LengthSquared() };

		if(distToTargetSq < m_attackRangeSq)
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt, true);
		else {
			// 타겟이 추격 가능 시 추격
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
		}
	}
}


GameServer::Contents::SoldierChaseState::SoldierChaseState(const float chaseRange, const float attackRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_CHASE }, m_chaseRangeSq{ chaseRange * chaseRange }, m_attackRangeSq{attackRange * attackRange}
{
}

GameServer::Contents::SoldierChaseState::~SoldierChaseState()
{
}

void GameServer::Contents::SoldierChaseState::Enter(const float dt)
{
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Enter SoldierChaseState", GetFSM()->GetOwner()->GetID()) << std::endl;
#endif
	
	 const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(!IsValidObj(target))
		return;

	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(ag)
		ag->SetDestPos(target->GetPosition());
}

void GameServer::Contents::SoldierChaseState::Exit(const float dt)
{
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Exit SoldierChaseState", GetFSM()->GetOwner()->GetID()) << std::endl;
#endif
}

void GameServer::Contents::SoldierChaseState::Update(const float dt)
{
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(!IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	// 추격
	const Vec3& targetPos{ target->GetPosition() };
	owner->LookAt(targetPos);

	const float distToTargetSq{ (target->GetPosition() - owner->GetPosition()).LengthSquared() };

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

	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(ag)
		ag->SetDestPos(targetPos);
}

GameServer::Contents::SoldierAttackState::SoldierAttackState(const float attackRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK }, m_accDTForAttack{ }, m_attackRangeSq{attackRange * attackRange}
{
}

GameServer::Contents::SoldierAttackState::~SoldierAttackState()
{
}

void GameServer::Contents::SoldierAttackState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Enter SoldierAttackState", id) << std::endl;
#endif
	m_accDTForAttack = 0.f;
	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(ag)
		ag->StopMove();
}

void GameServer::Contents::SoldierAttackState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Exit SoldierAttackState", id) << std::endl;
#endif
	m_accDTForAttack = 0.f;
}

void GameServer::Contents::SoldierAttackState::Update(const float dt)
{
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(!IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	owner->LookAt(target->GetPosition());
	m_accDTForAttack += dt;

	if(m_accDTForAttack < 1.f)
		return;

	m_accDTForAttack = 0.f;

	const float distToTargetSq{ (target->GetPosition() - owner->GetPosition()).LengthSquared() };
	if(distToTargetSq < m_attackRangeSq) {
		if(target->OnDamaged(owner, dt)) {
			const auto& p{ owner->GetPosition() };
#ifdef PRINT_SOLDIER_STATE_LOG
			std::cout << std::format("{}, {}, {}", p.x, p.y, p.z) << std::endl;
			std::cout << "Soldier Attack Success!" << std::endl;
#endif
		}
	}
	else {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
	}
}

GameServer::Contents::SoldierDeadState::SoldierDeadState(const float deadAnimTime)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_DEAD }, m_accDT{}, m_deadAnimTime{ deadAnimTime }
{

}

GameServer::Contents::SoldierDeadState::~SoldierDeadState()
{
}

void GameServer::Contents::SoldierDeadState::Enter(const float dt)
{
	m_accDT = 0.f;
	std::cout << std::format("ID = {}, Enter Soldier Dead State", GetFSM()->GetOwner()->GetID()) << std::endl;
}

void GameServer::Contents::SoldierDeadState::Exit(const float dt)
{
	m_accDT = 0.f;
	std::cout << std::format("ID = {}, Exit Soldier Dead State", GetFSM()->GetOwner()->GetID()) << std::endl;
}

void GameServer::Contents::SoldierDeadState::Update(const float dt)
{
	m_accDT += dt;
	
	// TODO: 죽는 애니메이션 시간동안 대기
	if(m_accDT >= m_deadAnimTime) {
#ifdef PRINT_SOLDIER_STATE_LOG
		std::cout << "Soldier Dead!" << std::endl;
#endif
		const auto owner{ GetFSM()->GetOwner() };
		const auto world{ owner->GetGameWorld() };
		world->RemoveGameObject(owner);
	}
}

