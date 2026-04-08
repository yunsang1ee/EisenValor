#include "pch.h"
#include "Soldier.h"

#include "GameWorld.h"
#include "Player.h"
#include "FSM.h"
#include "NavAgent.h"
GameServer::Contents::Soldier::Soldier(const FB_ENUMS::TEAM_TYPE teamType)
	:Creature{teamType, FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER}
{
}

GameServer::Contents::Soldier::~Soldier()
{
	std::cout << "~Soldier" << std::endl;
}

void GameServer::Contents::Soldier::OnCollisionEnter(Collider* const other)
{

}

void GameServer::Contents::Soldier::OnCollisionStay(Collider* const other)
{

}

void GameServer::Contents::Soldier::OnCollisionExit(Collider* const other)
{

}

void GameServer::Contents::Soldier::Update(const float dt)
{
	GameObject::Update(dt);
	auto pb{ ServerPackets::Make_SC_MOVE_PACKET(GetID(), GetTransform(), 0)};
	GetGameWorld()->Broadcast(std::move(pb));
}

void GameServer::Contents::Soldier::OnDeath()
{
	const auto& gameWorld{ GetGameWorld() };
	const auto fsm{ GetComponent<GameServer::Contents::FSM>() };
	const float dt{ gameWorld->GetGameWorldDT() };
	fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_DEAD, dt, true);
	const auto ag{ GetComponent<GameServer::Contents::NavAgent>() };
	if(ag)
		ag->Remove();
}

bool GameServer::Contents::Soldier::OnDamaged(std::shared_ptr<Creature> const attacker, const float dt)
{
	uint32 damage{};

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {
		auto attackerPlayer = std::static_pointer_cast<Player>(attacker);
		const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAtkInfo() };

		if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
			damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
		}
		else {
			damage = attackerAtkInfo.skillData->damage;
		}
	}
	else if(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER == attacker->GetObjType()) {
		damage = 10;
	}

	DecHP(damage);

	return true;
}