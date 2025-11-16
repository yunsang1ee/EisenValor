#include "pch.h"
#include "Creature.h"

Server::Contents::Creature::Creature(const FB_ENUMS::GAME_OBJECT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType)
	:GameObject{type, teamType}
{
}

Server::Contents::Creature::~Creature()
{
}

void Server::Contents::Creature::SetHp(const int32 hp) noexcept
{
	if(hp > m_stat.hp) {
		m_stat.hp = std::min(hp, 100);
	}
	// hp <= m_stat.hp
	else {
		m_stat.hp = std::max(0, hp);
	}
}

void Server::Contents::Creature::OnDamaged(std::shared_ptr<Creature> attacker, const int32 damaged)
{

}
