#include "pch.h"
#include "Soldier.h"

#include "GameWorld.h"
#include "Player.h"

Server::Contents::Soldier::Soldier(const FB_ENUMS::TEAM_TYPE teamType)
	:Creature{teamType, FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER}
{
}

Server::Contents::Soldier::~Soldier()
{
	std::cout << "~Soldier" << std::endl;
}

void Server::Contents::Soldier::OnCollisionEnter(Collider* const other)
{

}

void Server::Contents::Soldier::OnCollisionStay(Collider* const other)
{

}

void Server::Contents::Soldier::OnCollisionExit(Collider* const other)
{

}

void Server::Contents::Soldier::Update(const float dt)
{
	GameObject::Update(dt);
	auto pb{ ServerPackets::Make_SC_MOVE_PACKET(GetID(), GetPosInfo(), GetComponent<Server::Contents::FSM>()->GetCurState()->GetStateType(), 0) };
	GetGameWorld()->Broadcast(std::move(pb));
}

void Server::Contents::Soldier::OnDeath()
{
	const auto& gameWorld{ GetGameWorld() };
	gameWorld->RemoveGameObject(this);
}

bool Server::Contents::Soldier::OnDamaged(Creature* const attacker, const float dt)
{
	uint32 damage{};

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {
		auto attackerPlayer = static_cast<Player*>(attacker);
		const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAttackInfo() };

		if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
			damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
		}
		else {
			damage = attackerAtkInfo.skillData->damage;
		}
	}
	DecHP(damage);

	return true;
}