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
	:State(etou8(GENERAL_STATE_TYPE::IDLE))
{
}

Server::Contents::GeneralIdleState::~GeneralIdleState()
{
}

void Server::Contents::GeneralIdleState::Enter(const float dt)
{
	std::cout << "GENERAL IDLE ENTER" << std::endl;
}

void Server::Contents::GeneralIdleState::Exit(const float dt)
{
	std::cout << "GENERAL IDLE EXIT" << std::endl;
}

uint8 Server::Contents::GeneralIdleState::Update(const float dt)
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
					return etou8(GENERAL_STATE_TYPE::TRACE);
				}
			}
		}
	}

	return GetType();
}

Server::Contents::GeneralTraceState::GeneralTraceState()
	:State(etou8(GENERAL_STATE_TYPE::TRACE))
{
}

Server::Contents::GeneralTraceState::~GeneralTraceState()
{
}

void Server::Contents::GeneralTraceState::Enter(const float dt)
{
	std::cout << "GENERAL TRACE ENTER" << std::endl;
}

void Server::Contents::GeneralTraceState::Exit(const float dt)
{
	std::cout << "GENERAL TRACE EXIT" << std::endl;
}

uint8 Server::Contents::GeneralTraceState::Update(const float dt)
{
	const auto owner = std::static_pointer_cast<Server::Contents::NPC>(GetFSM()->GetOwner());

	if(auto target = (owner->GetTarget())) {
		const Vec3 targetPos = target->GetPos();
		Vec3 myPos = GetFSM()->GetOwner()->GetPos();

		Vec3 dist = targetPos - myPos;

		if(dist.Length() <= attackRange || dist.Length() >= 5.f) {
			return etou8(GENERAL_STATE_TYPE::IDLE);
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

		/*const auto troopController = owner->GetComponent<Server::Contents::TroopController>();
		const auto& soldiers = troopController->GetSoldiers();

		for(const auto& [id, soldier] : soldiers) {
			if(auto s = soldier.lock()) {
				const auto fsm = s->GetComponent<Server::Contents::FSM>();
				if(fsm->GetCurState()->GetType() != STATE_TYPE::WALK) {
					s->GetComponent<Server::Contents::FSM>()->ChangeState(STATE_TYPE::WALK);
				}
			}
		}*/
	}

	return GetType();
}