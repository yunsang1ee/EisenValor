#include "pch.h"
#include "GeneralIdleState.h"

#include "FSM.h"
#include "GameObject.h"
#include "GameRoom.h"
#include "Player.h"
#include "NPC.h"

void Server::Contents::GeneralIdleState::Enter()
{
	std::cout << "GENERAL IDLE ENTER" << std::endl;
}

void Server::Contents::GeneralIdleState::Exit()
{
	std::cout << "GENERAL IDLE EXIT" << std::endl;
}

void Server::Contents::GeneralIdleState::Update(const float dt)
{
	const auto& players = GetFSM()->GetOwner()->GetGameRoom()->GetPlayers();
	const Vec3 myPos = GetFSM()->GetOwner()->GetPos();

	for(const auto& [id, player]:players) {
		const Vec3 playerPos = player->GetPos();
		const float dist = (playerPos - myPos).Length();
		
		if(dist <= detectRange) {
			std::static_pointer_cast<Server::Contents::NPC>(GetFSM()->GetOwner())->SetTarget(player);
			GetFSM()->ChangeState(STATE_TYPE::WALK);
		}
	}
}