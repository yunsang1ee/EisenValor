#include "pch.h"
#include "TargetTraceNode.h"

#include "BehaviorTree.h"
#include "NPC.h"
#include "Player.h"
#include "GameRoom.h"

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::TargetTraceNode::DoAction(const float dt)
{
	if(auto target = std::static_pointer_cast<Server::Contents::NPC>(GetTree()->GetOwner())->GetTarget()) {
		const Vec3 targetPos = target->GetPos();
		Vec3 myPos = GetTree()->GetOwner()->GetPos();

		Vec3 dist = targetPos - myPos;

		if(dist.Length() <= m_attackRange || dist.Length() >= 5.f) {
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}

		dist.Normalize();

		myPos += dist * 3.f * dt;
		GetTree()->GetOwner()->SetPos(myPos);

		const uint32 id{ GetTree()->GetOwner()->GetID() };
		const Vec3 pos{ myPos };
		const Vec3 rot{ GetTree()->GetOwner()->GetRotation() };

		auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
		GetTree()->GetOwner()->GetGameRoom()->BroadcastToAll(std::move(pb));
		return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
	}

	return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
}
