#include "pch.h"
#include "GeneralState.h"
#include "FSM.h"
#include "GameObject.h"
#include "GameRoom.h"
#include "Player.h"
#include "NPC.h"
#include "TroopController.h"
#include "SoldierState.h"

Server::Contents::GeneralIdleState::GeneralIdleState()
	:State(STATE_TYPE::IDLE)
{
}

Server::Contents::GeneralIdleState::~GeneralIdleState()
{
}

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
	const auto room = GetFSM()->GetOwner()->GetGameRoom();
	if(room) {
		const auto& players = GetFSM()->GetOwner()->GetGameRoom()->GetPlayers();
		const Vec3 myPos = GetFSM()->GetOwner()->GetPos();

		for(const auto& [id, player] : players) {
			if(player) {
				const Vec3 playerPos = player->GetPos();
				const float dist = (playerPos - myPos).Length();

				if(dist <= detectRange) {
					std::static_pointer_cast<Server::Contents::NPC>(GetFSM()->GetOwner())->SetTarget(player);
					GetFSM()->ChangeState(STATE_TYPE::WALK);
				}
			}
		}
	}
}

Server::Contents::GeneralTraceState::GeneralTraceState()
	:State(STATE_TYPE::WALK)
{
}

Server::Contents::GeneralTraceState::~GeneralTraceState()
{
}

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
	const auto owner = std::static_pointer_cast<Server::Contents::NPC>(GetFSM()->GetOwner());

	if(auto target = (owner->GetTarget())) {
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
		const auto gameRoom = GetFSM()->GetOwner()->GetGameRoom();
		if(gameRoom) {
			gameRoom->ExecuteAsyncronously(&Server::Contents::GameRoom::Broadcast, std::move(pb));
		}

		const auto troopController = owner->GetComponent<Server::Contents::TroopController>();
		const auto& soldiers = troopController->GetSoldiers();

		for(const auto& [id, soldier] : soldiers) {
			if(auto s = soldier.lock()) {
				const auto fsm = s->GetComponent<Server::Contents::FSM>();
				if(fsm->GetCurState()->GetType() != STATE_TYPE::WALK) {
					s->GetComponent<Server::Contents::FSM>()->ChangeState(STATE_TYPE::WALK);
				}
				const auto walkState = std::static_pointer_cast<Server::Contents::SoldierWalkState>(fsm->GetCurState());
				if(walkState) {
					walkState->SetTargetPos(targetPos);
				}
			}
		}
	}
}