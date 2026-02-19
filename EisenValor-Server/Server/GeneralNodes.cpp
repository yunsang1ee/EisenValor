#include "pch.h"
#include "GeneralNodes.h"

#include "BehaviorTree.h"
#include "GameWorld.h"
#include "GameObject.h"
#include "OccupationZone.h"
#include "NavAgent.h"

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
				// std::cout << "SetDestPos!" << std::endl;
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
