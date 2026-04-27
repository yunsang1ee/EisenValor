#include "pch.h"
#include "SoldierStates.h"

#include "Player.h"
#include "FSM.h"
#include "NavAgent.h"
#include "GameWorld.h"
#include "Soldier.h"

#define PRINT_SOLDIER_STATE_LOG

// ============================================
//					IDLE
// ============================================
GameServer::Contents::SoldierIdleState::SoldierIdleState()
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_IDLE }, m_isSpawned{ false }, m_accDT{ 0.f }
{

}

GameServer::Contents::SoldierIdleState::~SoldierIdleState()
{

}

void GameServer::Contents::SoldierIdleState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Enter Soldier Idle", id) << std::endl;
#endif
	m_accDT = 0.f;
}

void GameServer::Contents::SoldierIdleState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Exit Soldier Idle", id) << std::endl;
#endif

	m_accDT = 0.f;
}

void GameServer::Contents::SoldierIdleState::Update(const float dt)
{
	m_accDT += dt;

	if(!m_isSpawned && m_accDT >= SPAWN_DELAY) {
		m_isSpawned = true;
		auto const fsm = GetFSM();
		fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	if(!m_isSpawned) return;

	const auto& owner = std::static_pointer_cast<Soldier>(GetFSM()->GetOwner());
	const auto world{ owner->GetGameWorld() };
	if(!world)
		return;

	std::shared_ptr<Creature> target;
	float closestDistSq{ 5.f };

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


// ============================================
//					MOVE
// ============================================
GameServer::Contents::SoldierMoveState::SoldierMoveState(const float viewRange, const Vec3& destPos)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_MOVE }, m_viewRangeSq{ viewRange * viewRange }, m_accDTForSearch{ 0.f }, m_destPos{ destPos }
{
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

	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(ag) {
		ag->SetDestPos(m_destPos);
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
	if(!IsValidObj(owner))
		return;

	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	const auto& pos{ owner->GetPosition() };

	auto diff = m_destPos - pos;
	const float distXZSq = diff.x * diff.x + diff.z * diff.z;
	if(distXZSq < 0.5f * 0.5f) {
		owner->SetPosition(m_destPos);
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE, dt, true);
		ag->StopMove();
		std::cout << "Arrive!" << std::endl;
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


// ============================================
//					CHASE
// ============================================
GameServer::Contents::SoldierChaseState::SoldierChaseState(const float chaseRange, const float attackRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_CHASE }, m_chaseRangeSq{ chaseRange * chaseRange }, m_attackRangeSq{ attackRange * attackRange }, m_chaseTransitionRangeSq{ (attackRange * 1.2f) * (attackRange * 1.2f) }
	, m_accDTForChase{ 0.f }
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

	m_accDTForChase = 0.f;

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
	m_accDTForChase = 0.f;
}

void GameServer::Contents::SoldierChaseState::Update(const float dt)
{
	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(!IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	const float distToTargetSq{ (target->GetPosition() - owner->GetPosition()).LengthSquared() };

	if(distToTargetSq > m_chaseRangeSq) {
		owner->SetTarget(nullptr);
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	if(distToTargetSq < m_attackRangeSq) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt, true);
		return;
	}

	m_accDTForChase += dt;
	if(m_accDTForChase < CHASE_UPDATE_INTERVAL)
		return;

	m_accDTForChase = 0.f;

	const Vec3& targetPos{ target->GetPosition() };
	owner->LookAt(targetPos);

	const auto ag{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(ag)
		ag->SetDestPos(targetPos);
}


// ============================================
//					ATTACK
// ============================================
GameServer::Contents::SoldierAttackState::SoldierAttackState(const float attackRange)
	:State{ FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK }, m_accDTForAttack{ }, m_attackRangeSq{ attackRange * attackRange }, m_hitFired{ false }, m_attackStarted{ false }
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

	m_hitFired = false;
	m_attackStarted = false;
}

void GameServer::Contents::SoldierAttackState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint64 id{ owner->GetID() };
#ifdef PRINT_SOLDIER_STATE_LOG
	std::cout << std::format("ID = {}, Exit SoldierAttackState", id) << std::endl;
#endif
	m_accDTForAttack = 0.f;

	m_hitFired = false;
	m_attackStarted = false;
}

void GameServer::Contents::SoldierAttackState::Update(const float dt)
{
	constexpr float ATTACK_CYCLE = 1.7f;
	constexpr float HIT_FRAME_DELAY = 0.5f;

	const auto owner{ std::static_pointer_cast<Soldier>(GetFSM()->GetOwner()) };
	const auto target{ owner->GetTarget() };

	if(!IsValidObj(target)) {
		GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE, dt, true);
		return;
	}

	owner->LookAt(target->GetPosition());
	m_accDTForAttack += dt;

	if(!m_attackStarted) {
		m_attackStarted = true;
		auto pb{ ServerPackets::Make_SC_SOLDIER_ATTACK_PACKET(owner->GetID()) };
		owner->GetGameWorld()->Broadcast(std::move(pb));
	}

	if(!m_hitFired && m_accDTForAttack >= HIT_FRAME_DELAY) {
		m_hitFired = true;

		const float distToTargetSq{ (target->GetPosition() - owner->GetPosition()).LengthSquared() };
		if(distToTargetSq <= m_attackRangeSq) {
			target->OnAttacked(owner, dt, false);

			auto weakTarget = std::weak_ptr<Creature>(target);
			owner->GetGameWorld()->AddTimedEvent([weakTarget]()
				{
					auto t = weakTarget.lock();
					if(t) t->BroadcastUpdateVital();
				}, 0.05f);
		}
		else {
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
			return;
		}
	}
	
	if(m_accDTForAttack >= ATTACK_CYCLE) {
		m_accDTForAttack -= ATTACK_CYCLE;
		m_hitFired = false;

		const float distToTargetSq{ (target->GetPosition() - owner->GetPosition()).LengthSquared() };
		if(distToTargetSq > m_attackRangeSq) {
			m_attackStarted = false;
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt, true);
			return;
		}

		m_attackStarted = true;
		auto pb{ ServerPackets::Make_SC_SOLDIER_ATTACK_PACKET(owner->GetID()) };
		owner->GetGameWorld()->Broadcast(std::move(pb));
	}
}


// ============================================
// 					  DEAD
// ============================================
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

	if(m_accDT >= m_deadAnimTime) {
#ifdef PRINT_SOLDIER_STATE_LOG
		std::cout << "Soldier Dead!" << std::endl;
#endif
		const auto owner{ GetFSM()->GetOwner() };
		const auto world{ owner->GetGameWorld() };
		world->RemoveGameObject(owner);
	}
}