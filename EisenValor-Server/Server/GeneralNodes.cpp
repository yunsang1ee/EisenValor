#include "pch.h"
#include "GeneralNodes.h"

#include "GameWorld.h"
#include "GameObject.h"
#include "OccupationZone.h"
#include "NavAgent.h"
#include "General.h"
#include "FSM.h"

// #define PRINT_GENERAL_NODE_LOG

// =====================================================
//				     CONDITION NODES
// =====================================================
bool GameServer::Contents::IsInOccupationZone::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto& ownerPos{ owner->GetPosition() };

	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };
	for(const auto& [id, o] : gameObjectGroup) {
		auto const obj{ o.get() };
		if(false == IsValidObj(o)) continue;

		auto const oz{ static_cast<OccupationZone*>(obj->GetScript(obj->GetName())) };
		if(oz && oz->IsInOccupationZone(ownerPos)) {
			return true;
		}
	}
	return false;
}

bool GameServer::Contents::IsInUnoccupiedZone::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto& ownerPos{ owner->GetPosition() };

	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };
	for(const auto& [id, o] : gameObjectGroup) {
		auto const obj{ o.get() };
		if(false == IsValidObj(o)) continue;

		auto const oz{ static_cast<OccupationZone*>(obj->GetScript(obj->GetName())) };
		if(oz
			&& FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == oz->GetStateType() && oz->IsInOccupationZone(ownerPos)) {
			return true;
		}
	}
	return false;
}

bool GameServer::Contents::AreAllZonesOccupied::Check(const float dt)
{
	const auto owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };

	bool hasAnyZone{ false };
	for(const auto& [id, o] : gameObjectGroup) {
		auto const obj{ o.get() };
		if(false == IsValidObj(o)) continue;

		auto const oz{ static_cast<OccupationZone*>(obj->GetScript(obj->GetName())) };
		if(!oz) continue;

		hasAnyZone = true;
		if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == oz->GetStateType()) {
			return false;
		}
	}
	return hasAnyZone;
}

bool GameServer::Contents::IsRespawnReady::Check(const float dt)
{
	m_accDTForRespawn += dt;
	const auto owner{ GetOwner() };
	const auto& statInfo{ owner->GetStat() };

	if(m_accDTForRespawn >= statInfo.respawnTimeSec) {
		m_accDTForRespawn = 0.f;
		return true;
	}

	return false;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::ChangeState::DoAction(const float dt)
{
	auto fsm = GetOwner()->GetComponent<FSM>();
	fsm->ChangeState(m_stateType, dt, true);
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::Respawn::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	owner->OnRespawn();
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

bool GameServer::Contents::HasTarget::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	return IsValidObj(target);
}

bool GameServer::Contents::IsTargetLost::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	return false == IsValidObj(target);
}

bool GameServer::Contents::IsTargetInDetectionRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;

	const auto& objData{ owner->GetGameObjectData() };
	constexpr float leashRange{ 12.f };
	return owner->IsTargetInRange(target, leashRange * leashRange);
}

bool GameServer::Contents::IsTargetInCombatRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;

	const auto& objData{ owner->GetGameObjectData() };
	constexpr float range{ 5.f };
	return owner->IsTargetInRange(target, range * range);
}

bool GameServer::Contents::IsTargetInAttackRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;
	constexpr float range{ 5.f };
	return owner->IsTargetInRange(target, range * range);
}

bool GameServer::Contents::IsAttackCooldownReady::Check(const float dt)
{
	if(m_cycleSec < 0.f) {
		std::uniform_real_distribution<float> dist{ m_minSec, m_maxSec };
		m_cycleSec = dist(mersenne);
	}

	m_acc += dt;
	if(m_acc >= m_cycleSec) {
		m_acc = 0.f;
		std::cout << "Attack Cooldown Ready! Next Cooldown: " << m_cycleSec << " sec" << std::endl;
		std::uniform_real_distribution<float> dist{ m_minSec, m_maxSec };
		m_cycleSec = dist(mersenne);
		return true;
	}
	return false;
}

bool GameServer::Contents::IsStunOver::Check(const float dt)
{
	m_acc += dt;
	const auto owner{ GetOwner() };
	const auto& objData{ owner->GetGameObjectData() };
	constexpr float stunSec{ 2.f };
	return m_acc >= stunSec;
}

// =====================================================
// 				    ACTION NODES
// =====================================================
GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::FindEnemy::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	const auto& objData{ owner->GetGameObjectData() };
	constexpr float detectionRange{ 12.f };
	constexpr float detectionRangeSq{ detectionRange * detectionRange };
	if(detectionRangeSq <= 0.f) return BEHAVIOR_NODE_STATUS::FAIL;

	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroups{ world->GetGameObjectGroups() };
	const auto& myPos{ owner->GetPosition() };

	std::shared_ptr<Creature> nearestEnemy;
	float nearestDistSq{ std::numeric_limits<float>::max() };

	for(int i = 0; i < gameObjectGroups.size(); ++i) {
		if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL != i &&
			FB_ENUMS::GAME_OBJECT_TYPE_PLAYER != i &&
			FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER != i)
			continue;

		for(const auto& [id, o] : gameObjectGroups[i]) {
			if(false == IsValidObj(o)) continue;
			if(id == owner->GetID()) continue;
			if(o->GetTeamType() == owner->GetTeamType()) continue;

			const float distSq{ GetDistSq(myPos, o->GetPosition()) };
			if(distSq <= detectionRangeSq && distSq < nearestDistSq) {
				nearestDistSq = distSq;
				nearestEnemy = std::static_pointer_cast<Creature>(o);
			}
		}
	}

	if(nearestEnemy) {
		owner->SetTarget(nearestEnemy);
		return BEHAVIOR_NODE_STATUS::SUCCESS;
	}
	return BEHAVIOR_NODE_STATUS::FAIL;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::ClearTarget::DoAction(const float dt)
{
	GetOwner()->SetTarget(nullptr);
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::MoveToTarget::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return BEHAVIOR_NODE_STATUS::FAIL;

	auto const navAgent{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(navAgent) navAgent->SetDestPos(target->GetPosition());
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::StopMoving::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	auto const navAgent{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(navAgent) navAgent->StopMove();
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::LookAtTarget::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return BEHAVIOR_NODE_STATUS::FAIL;

	owner->LookAt(target->GetPosition());
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::SetStance::DoAction(const float dt)
{
	GetOwner()->SetStanceType(m_stance);
	if(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT == m_stance) {
#ifdef PRINT_GENERAL_NODE_LOG
		std::cout << "SetStance: COMBAT" << std::endl;
#endif
	}
	else
		{
#ifdef PRINT_GENERAL_NODE_LOG
		std::cout << "SetStance: NEUTRAL" << std::endl;
#endif
	}
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::RandomizeAttackDir::DoAction(const float dt)
{
	m_acc += dt;
	if(m_acc < m_intervalSec) return BEHAVIOR_NODE_STATUS::SUCCESS;
	m_acc = 0.f;

	auto const owner{ GetOwner() };

	std::uniform_int_distribution<int> dist{
		FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP,
		FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_RIGHT };
	const auto dir{ static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>(dist(mersenne)) };

	if(owner->GetAtkInfo().dir == dir) return BEHAVIOR_NODE_STATUS::SUCCESS;

	owner->SetAtkDir(dir);

	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), etou8(dir)) };
	owner->GetGameWorld()->Broadcast(std::move(pb));

	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::WanderAroundTarget::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return BEHAVIOR_NODE_STATUS::FAIL;

	m_acc += dt;
	if(m_acc < m_intervalSec) return BEHAVIOR_NODE_STATUS::SUCCESS;
	m_acc = 0.f;

	const Vec3& ownerPos{ owner->GetPosition() };
	const Vec3& targetPos{ target->GetPosition() };

	// owner 기준 forward(타겟 방향) / right 축
	const float fwdX{ targetPos.x - ownerPos.x };
	const float fwdZ{ targetPos.z - ownerPos.z };
	const float fwdLenSq{ fwdX * fwdX + fwdZ * fwdZ };
	if(fwdLenSq < 0.0001f) return BEHAVIOR_NODE_STATUS::SUCCESS;
	const float invLen{ 1.f / std::sqrtf(fwdLenSq) };
	const float fX{ fwdX * invLen };
	const float fZ{ fwdZ * invLen };
	const float rX{ fZ };
	const float rZ{ -fX };

	// FWD/BWD/LFT/RGT 중 직전과 다른 방향을 선택
	constexpr int DIR_COUNT{ 4 };
	int dirIdx;
	if(m_lastDirIdx < 0) {
		std::uniform_int_distribution<int> dist{ 0, DIR_COUNT - 1 };
		dirIdx = dist(mersenne);
	}
	else {
		std::uniform_int_distribution<int> dist{ 0, DIR_COUNT - 2 };
		dirIdx = dist(mersenne);
		if(dirIdx >= m_lastDirIdx) ++dirIdx;
	}
	m_lastDirIdx = dirIdx;

	std::uniform_real_distribution<float> stepDist{ m_minDist, m_maxDist };
	const float step{ stepDist(mersenne) };

	float dX{ 0.f }, dZ{ 0.f };
	FB_ENUMS::MOVE_DIRECTION_TYPE moveDir{ FB_ENUMS::MOVE_DIRECTION_TYPE_FWD };
	switch(dirIdx) {
		case 0:
		{
			dX = fX * step;
			dZ = fZ * step;
			moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_FWD;
#ifdef PRINT_GENERAL_NODE_LOG
			std::cout << "FWD" << std::endl;
#endif
			break;
		}
		case 1:
		{
			dX = -fX * step;
			dZ = -fZ * step;
			moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_BWD;
#ifdef PRINT_GENERAL_NODE_LOG
			std::cout << "BWD" << std::endl;
#endif
			break;
		}
		case 2:
		{
			dX = -rX * step;
			dZ = -rZ * step;
			moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_LFT;
#ifdef PRINT_GENERAL_NODE_LOG
			std::cout << "LFT" << std::endl;
#endif
			break;
		}
		case 3:
		{
			dX = rX * step;
			dZ = rZ * step;
			moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_RGT;
#ifdef PRINT_GENERAL_NODE_LOG
			std::cout << "RGT" << std::endl;
#endif
			break;
		}
	}

	const Vec3 dest{ ownerPos.x + dX, ownerPos.y, ownerPos.z + dZ };

	owner->SetMoveDir(moveDir);
	auto const navAgent{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(navAgent) navAgent->SetDestPos(dest);

	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::Attack::DoAction(const float dt)
{
	constexpr float HIT_FRAME_DELAY{ 0.5f };

	auto const owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return BEHAVIOR_NODE_STATUS::FAIL;
	if(false == owner->IsTargetInAttackRange(target)) return BEHAVIOR_NODE_STATUS::FAIL;

	const auto& atkInfo{ owner->GetAtkInfo() };
	if(nullptr == atkInfo.skillData) return BEHAVIOR_NODE_STATUS::FAIL;

	const FB_STRUCTS::GeneralAttackInfo info{ static_cast<FB_ENUMS::GENERAL_ATTACK_TYPE>(atkInfo.skillData->skillTypeID),atkInfo.dir };
	auto pb{ ServerPackets::Make_SC_GENERAL_ATTACK_PACKET(owner->GetID(), info) };
	auto const world{ owner->GetGameWorld() };
	world->Broadcast(std::move(pb));

	std::weak_ptr<Creature> weakOwner{ std::static_pointer_cast<Creature>(owner) };
	std::weak_ptr<Creature> weakTarget{ target };
	world->AddTimedEvent([weakOwner, weakTarget, dt]()
		{
			auto o = weakOwner.lock();
			auto t = weakTarget.lock();
			if(!IsValidObj(o) || !IsValidObj(t)) return;
			t->OnDamaged(o, dt);
		}, HIT_FRAME_DELAY);

	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::WaitOnce::DoAction(const float dt)
{
	if(m_done) return BEHAVIOR_NODE_STATUS::FAIL;

	m_acc += dt;
	if(m_acc < m_durationSec) return BEHAVIOR_NODE_STATUS::RUNNING;

	m_done = true;
	return BEHAVIOR_NODE_STATUS::FAIL;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::SetMaxSpeed::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	auto const navAgent{ owner->GetComponent<GameServer::Contents::NavAgent>() };
	if(navAgent && navAgent->GetMaxSpeed() != m_maxSpeed) {
		navAgent->SetMaxSpeed(m_maxSpeed);
	}
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::MoveToOZ::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };

	std::vector<GameObject*> unoccupied;
	std::vector<GameObject*> all;
	unoccupied.reserve(gameObjectGroup.size());
	all.reserve(gameObjectGroup.size());

	for(const auto& [id, o] : gameObjectGroup) {
		auto const obj{ o.get() };
		if(false == IsValidObj(o)) continue;

		auto const oz{ static_cast<OccupationZone*>(obj->GetScript(obj->GetName())) };
		if(!oz) continue;

		all.push_back(obj);
		if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == oz->GetStateType()) {
			unoccupied.push_back(obj);
		}
	}

	// 점령되지 않은 점령지가 있으면 그 중 랜덤, 없으면 전체 점령지 중 랜덤.
	const auto& pool{ unoccupied.empty() ? all : unoccupied };
	if(pool.empty()) return BEHAVIOR_NODE_STATUS::FAIL;

	GameObject* target{ nullptr };
	if(pool.size() == 1) {
		target = pool.front();
	}
	else {
		std::uniform_int_distribution<size_t> dist{ 0, pool.size() - 1 };
		target = pool[dist(mersenne)];
	}

	owner->GetComponent<GameServer::Contents::NavAgent>()->SetDestPos(target->GetPosition());
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}
