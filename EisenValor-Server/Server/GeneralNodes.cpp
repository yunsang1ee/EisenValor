#include "pch.h"
#include "GeneralNodes.h"

#include "GameWorld.h"
#include "GameObject.h"
#include "OccupationZone.h"
#include "NavAgent.h"
#include "General.h"
#include "FSM.h"

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

// =====================================================
// === 순수 조건 노드                                ===
// =====================================================

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
	const float range{ objData ? objData->enemyDetectionRange : 0.f };
	return owner->IsTargetInRange(target, range * range);
}

bool GameServer::Contents::IsTargetInCombatRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;

	const auto& objData{ owner->GetGameObjectData() };
	const float range{ objData ? objData->enemyCombatRange : 0.f };
	return owner->IsTargetInRange(target, range * range);
}

bool GameServer::Contents::IsTargetInAttackRange::Check(const float dt)
{
	const auto owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return false;
	// 전방뿐만 아니라 360도 범위로 공격 탐지하게 변경해야함.
	return owner->IsTargetInAttackRange(target);
}

bool GameServer::Contents::IsAttackCooldownReady::Check(const float dt)
{
	m_acc += dt;
	const auto owner{ GetOwner() };
	const auto& objData{ owner->GetGameObjectData() };
	const float cycleSec{ objData ?5.f : 1.f };
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
// === 액션 노드                                     ===
// =====================================================

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::FindEnemy::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	const auto& objData{ owner->GetGameObjectData() };
	const float detectionRangeSq{ objData ? 3.f * 3.f : 0.f };
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

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::Attack::DoAction(const float dt)
{
	auto const owner{ GetOwner() };
	const auto target{ owner->GetTarget() };
	if(false == IsValidObj(target)) return BEHAVIOR_NODE_STATUS::FAIL;

	if(owner->IsTargetInAttackRange(target)) {
		target->OnDamaged(owner, dt);
		// TODO: SC_GENERAL_ATTACK_PACKET 브로드캐스트해서 클라이언트에서 공격 애메이션 재생하게 해야할듯
	}
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
