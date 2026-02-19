#include "pch.h"
#include "GeneralNodes.h"

#include "BehaviorTree.h"
#include "GameWorld.h"
#include "GameObject.h"
#include "OccupationZone.h"
#include "NavAgent.h"
#include "General.h"

// ====================================
//		  GENERAL_ROAMING_STATE
// ====================================
Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::ActionFindOZ::DoAction(const float dt)
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
				tree->GetBlackboard()->SetValue("OZ_ID", o->GetID());
				return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
			}
		}
	}

	return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::ActionMoveToOZ::DoAction(const float dt)
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
			}
			else {
				owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(ozPos);
			}

			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
	}
	else {
		std::cout << "oz loss!" << std::endl;
	}

	return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
}



// ====================================
//		  GENERAL_DUELING_STATE
// ====================================
bool Server::Contents::ConditionIsTargetAttacking::Check(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);
	
	if(-1 != targetID) {
	
		auto target{ world->FindObjectByID(targetID) };
		if(target) {
			owner->SetLook(target->GetPos());
			const auto targetObjType{ target->GetObjType() };
			if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == targetObjType) {
				auto const fsm{ target->GetComponent<Server::Contents::FSM>() };
				const auto stateType{ fsm->GetCurState()->GetStateType() };

				if(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK == stateType || FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY == stateType || FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY  == stateType)
					return true;
			}
			else if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == targetObjType) {

			}
		}
	}

	return false;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::ActionDefense::DoAction(const float dt)
{
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };

	return BEHAVIOR_NODE_STATUS();
}
