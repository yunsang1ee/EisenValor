#include "pch.h"
#include "GeneralNodes.h"

#include "GameWorld.h"
#include "GameObject.h"
#include "OccupationZone.h"
#include "NavAgent.h"
#include "General.h"
#include "FSM.h"

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

	// TODO: 범위 벗어남 조건 추가할듯

	return false == IsValidObj(target);
}

bool GameServer::Contents::IsTargetInDetectionRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;

	const auto& objData{ owner->GetGameObjectData() };
	constexpr float leashRange{ 6.f };
	return owner->IsTargetInRange(target, leashRange * leashRange);
}

bool GameServer::Contents::IsTargetInCombatRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;

	const auto& objData{ owner->GetGameObjectData() };
	constexpr float range{3.f};
	return owner->IsTargetInRange(target, range * range);
}

bool GameServer::Contents::IsTargetInAttackRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;
	constexpr float range{ 3.f };
	return owner->IsTargetInRange(target, range * range);
}

bool GameServer::Contents::IsAttackCooldownReady::Check(const float dt)
{
	m_acc += dt;
	const auto owner{ GetOwner() };
	const auto& objData{ owner->GetGameObjectData() };
	constexpr float cycleSec{5.f};
	if(m_acc >= cycleSec) {
		m_acc = 0.f;
		return true;
	}
	return false;
}

bool GameServer::Contents::IsStunOver::Check(const float dt)
{
	m_acc += dt;
	const auto owner{ GetOwner() };
	const auto& objData{ owner->GetGameObjectData() };
	const float stunSec{ objData ? objData->stunDelay / 1000.f : 1.f };
	return m_acc >= stunSec;
}

// =====================================================
// 				    ACTION NODES
// =====================================================
GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::FindEnemy::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	const auto& objData{ owner->GetGameObjectData() };
	constexpr float detectionRange{ 3.f };
	constexpr float detectionRangeSq{ detectionRange * detectionRange };
	if(detectionRangeSq <= 0.f) return BEHAVIOR_NODE_STATUS::FAIL;

	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroups{ world->GetGameObjectGroups() };
	const auto& myPos{ owner->GetPosition() };

	std::shared_ptr<Creature> nearestEnemy;
	float nearestDistSq{ std::numeric_limits<float>::max() };

	for(int i = 0; i < gameObjectGroups.size(); ++i) {
		if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL != i && FB_ENUMS::GAME_OBJECT_TYPE_PLAYER != i)
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

	std::uniform_real_distribution<float> angleDist{ 0.f, 6.2831853f };
	std::uniform_real_distribution<float> distDist{ m_minDist, m_maxDist };
	const float angle{ angleDist(mersenne) };
	const float dist{ distDist(mersenne) };

	const Vec3& ownerPos{ owner->GetPosition() };
	const Vec3& targetPos{ target->GetPosition() };
	const Vec3 dest{
		targetPos.x + std::cosf(angle) * dist,
		targetPos.y,
		targetPos.z + std::sinf(angle) * dist };

	// 타겟 기준 forward/right 축으로 이동 방향 분류
	const float fwdX{ targetPos.x - ownerPos.x };
	const float fwdZ{ targetPos.z - ownerPos.z };
	const float fwdLenSq{ fwdX * fwdX + fwdZ * fwdZ };
	if(fwdLenSq > 0.0001f) {
		const float invLen{ 1.f / std::sqrtf(fwdLenSq) };
		const float fX{ fwdX * invLen };
		const float fZ{ fwdZ * invLen };
		const float rX{ fZ };
		const float rZ{ -fX };

		const float mX{ dest.x - ownerPos.x };
		const float mZ{ dest.z - ownerPos.z };
		const float fwdDot{ mX * fX + mZ * fZ };
		const float rgtDot{ mX * rX + mZ * rZ };

		FB_ENUMS::MOVE_DIRECTION_TYPE moveDir;
		if(std::fabs(fwdDot) >= std::fabs(rgtDot)) {
			moveDir = (fwdDot >= 0.f) ? FB_ENUMS::MOVE_DIRECTION_TYPE_FWD : FB_ENUMS::MOVE_DIRECTION_TYPE_BWD;
		}
		else {
			moveDir = (rgtDot >= 0.f) ? FB_ENUMS::MOVE_DIRECTION_TYPE_RGT : FB_ENUMS::MOVE_DIRECTION_TYPE_LFT;
		}
		owner->SetMoveDir(moveDir);
	}

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

	const FB_STRUCTS::GeneralAttackInfo info{static_cast<FB_ENUMS::GENERAL_ATTACK_TYPE>(atkInfo.skillData->skillTypeID),atkInfo.dir};
	auto pb{ ServerPackets::Make_SC_GENERAL_ATTACK_PACKET(owner->GetID(), info) };
	auto const world{ owner->GetGameWorld() };
	world->Broadcast(std::move(pb));

	std::weak_ptr<Creature> weakOwner{ std::static_pointer_cast<Creature>(owner) };
	std::weak_ptr<Creature> weakTarget{ target };
	world->AddTimedEvent([weakOwner, weakTarget, dt]() {
		auto o = weakOwner.lock();
		auto t = weakTarget.lock();
		if(!IsValidObj(o) || !IsValidObj(t)) return;
		t->OnDamaged(o, dt);
	}, HIT_FRAME_DELAY);

	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::MoveToOZ::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };

	for(const auto& [id, o] : gameObjectGroup) {
		auto const obj{ o.get() };
		if(false == IsValidObj(o)) continue;

		std::bernoulli_distribution dist{ 0.5 };
		std::string_view ozName{ dist(mersenne) ? "A" : "B" };

		auto const oz{ static_cast<OccupationZone*>(obj->GetScript(ozName.data())) };
		if(oz && FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == oz->GetStateType()) {
			owner->GetComponent<GameServer::Contents::NavAgent>()->SetDestPos(obj->GetPosition());
			return BEHAVIOR_NODE_STATUS::SUCCESS;
		}
	}
	return BEHAVIOR_NODE_STATUS::FAIL;
}
