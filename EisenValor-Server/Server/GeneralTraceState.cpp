#include "pch.h"
#include "GeneralTraceState.h"

#include "FSM.h"
#include "NPC.h"
#include "Player.h"
#include "GameRoom.h"

void Server::Contents::GeneralTraceState::Enter()
{
	std::cout << "GENERAL TRACE ENTER" << std::endl;
}

void Server::Contents::GeneralTraceState::Exit()
{
	std::cout << "GENERAL TRACE EXIT" << std::endl;
}

void Server::Contents::GeneralTraceState::Update(const float dt)
{
	if(auto target = std::static_pointer_cast<Server::Contents::NPC>(GetFSM()->GetOwner())->GetTarget()) {
		const Vec3 targetPos = target->GetPos();
		Vec3 myPos = GetFSM()->GetOwner()->GetPos();

		Vec3 dist = targetPos - myPos;

		if(dist.Length() <= attackRange || dist.Length() >= 5.f) {
			GetFSM()->ChangeState(STATE_TYPE::IDLE);
		}

		dist.Normalize();

		myPos += dist * 3.f * dt;
		GetFSM()->GetOwner()->SetPos(myPos);

		const uint32 id{ GetFSM()->GetOwner()->GetID() };
		const Vec3 pos{ myPos };
		const Vec3 rot{ GetFSM()->GetOwner()->GetRotation() };

		auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
		GetFSM()->GetOwner()->GetGameRoom()->Broadcast(std::move(pb));
	}
}
