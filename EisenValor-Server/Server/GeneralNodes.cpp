#include "pch.h"
#include "GeneralNodes.h"

#include "BehaviorTree.h"
#include "GameWorld.h"
#include "GameObject.h"
#include "OccupationZone.h"
#include "NavAgent.h"
#include "General.h"

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::FindOZ::DoAction(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	auto const world{ owner->GetGameWorld() };

	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };

	for(const auto& [id, o] : gameObjectGroup) {
		auto const obj{ o.get() };

		if(obj->GetTeamType() == owner->GetTeamType()) continue;

		auto oz{ static_cast<OccupationZone*>(obj->GetScript("OZ")) };
		if(oz) {
			if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == oz->GetStateType()) {
				owner->SetLook(obj->GetPos());
				tree->GetBlackboard()->SetValue("OZ_ID", o->GetID());
				return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
			}
		}
	}

	return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::MoveToOZ::DoAction(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	const auto& ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };
	auto const ozObj{ world->FindObjectByID(tree->GetBlackboard()->GetValue<uint32>("OZ_ID")) };

	if(ozObj) {
		const auto& ozPos{ ozObj->GetPos() };

		auto oz{ static_cast<OccupationZone*>(ozObj->GetScript("OZ")) };
		if(oz) {
			const float distToOzSq{ (ozPos - ownerPos).LengthSquared() };

			if(distToOzSq <= oz->GetRangeSq()) {
				std::cout << "In OZ!" << std::endl;
				return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
			}
			else {
				owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(ozPos);
			}
			return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
		}
	}
	else {
		std::cout << "oz loss!" << std::endl;
	}

	return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
}


bool Server::Contents::IsTargetAttacking::Check(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>( tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);

	if(-1 == targetID)
		return false;
	
	auto target{ world->FindObjectByID(targetID) };

	if(nullptr == target || false == target->IsActive())
		return false;

	owner->SetLook(target->GetPos());
	
	const auto targetObjType{ target->GetObjType() };
	
	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == targetObjType) {
		auto const fsm{ target->GetComponent<Server::Contents::FSM>() };
		const auto stateType{ fsm->GetCurState()->GetStateType() };

		if(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK == stateType || FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY == stateType || FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY == stateType) {
			owner->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT);
			auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(owner->GetID(), owner->GetStanceType()) };
			world->Broadcast(std::move(pb));

			return true;
		}
	}
	else if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == targetObjType) {

	}

	return true;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::MatchGuard::DoAction(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);

	if(-1 == targetID)
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	auto target{ static_cast<General*>(world->FindObjectByID(targetID)) };

	if(nullptr == target || false == target->IsActive())
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	const auto& targetAtkInfo{ target->GetAtkInfo() };

	auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), targetAtkInfo.dir) };
	world->Broadcast(std::move(pb));
	
	return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::Parrying::DoAction(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	auto const bb{ tree->GetBlackboard() };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);

	if(-1 == targetID)
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	auto target{ static_cast<General*>(world->FindObjectByID(targetID)) };

	if(nullptr == target || false == target->IsActive())
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	const auto& targetAtkInfo{ target->GetAtkInfo() };
	const auto targetAttackStartFrame{ targetAtkInfo.startPreDelay };
	const uint64 currentFrame = world->GetGameWorldFrameCount();

	uint64 elapsed = currentFrame - targetAttackStartFrame;
	const uint64 parryWindowStart = 25;
	const uint64 parryWindowEnd = 35;

	if(elapsed >= parryWindowStart && elapsed <= parryWindowEnd) {
		int chance = rand() % 100;
		int botLevelSkill = 80;

		if(chance < botLevelSkill) {
			if(target) {
				if(target->GetObjType() == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) {
					target->GetComponent<Server::Contents::FSM>()->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_STUN, true);
				}

				return BEHAVIOR_NODE_STATUS::SUCCESS;
			}
		}
	}

	// 아직 타이밍이 아니거나 실패했다면 RUNNING을 주어 다음 프레임에 다시 체크하게 함
	return BEHAVIOR_NODE_STATUS::RUNNING;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::DefaultAttack::DoAction(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);
	if(-1 == targetID)
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	auto obj{ world->FindObjectByID(targetID) };
	if(!obj || false == obj->IsActive() || !obj->IsCreature())
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	m_accDT += dt;
	if(m_accDT >= 1.f) {
		std::cout << std::format("accDT: {}, dt:{}", m_accDT, dt) << std::endl;
		m_accDT = 0.f;
		auto target{ static_cast<Server::Contents::Creature*>(obj) };

		const uint64 worldFrame{ world->GetGameWorldFrameCount() };

		const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dir{ FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_LEFT };
		const FB_ENUMS::GENERAL_ATTACK_TYPE atkType{ FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT };

		const SkillData* const skillData{ MANAGER(GameDataManager)->GetSkillData(atkType) };
		owner->SetAtkInfo(AttackInfo{ skillData, dir, worldFrame });
		owner->DecStamina(skillData->staminaCost, true);
		if(target->OnDamaged(owner, dt)) {
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
	}

	return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
}
