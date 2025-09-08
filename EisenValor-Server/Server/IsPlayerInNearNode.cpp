#include "pch.h"
#include "IsPlayerInNearNode.h"

#include "BehaviorTree.h"
#include "GameRoom.h"
#include "Player.h"
#include "NPC.h"
bool Server::Contents::IsPlayerInNearNode::Check()
{
	const auto& players = GetTree()->GetOwner()->GetGameRoom()->GetPlayers();
	const Vec3 myPos = GetTree()->GetOwner()->GetPos();

	for(const auto& [id, player] : players) {
		const Vec3 playerPos = player->GetPos();
		const float dist = (playerPos - myPos).Length();

		if(dist <= m_detectionRange) {
			std::static_pointer_cast<Server::Contents::NPC>(GetTree()->GetOwner())->SetTarget(player);
			// std::cout << "dist In Range!" << std::endl;
			return true;
		}
	}

	return false;
}
