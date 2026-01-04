#include "pch.h"
#include "Creature.h"

Server::Contents::Creature::Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type)
	:GameObject(teamType, type), m_alive{true}
{
}

Server::Contents::Creature::~Creature()
{
}

void Server::Contents::Creature::SetHp(const uint32 hp) noexcept
{
	if(hp > m_statInfo.hp) {
		m_statInfo.hp = std::min(hp, (uint32)100);
	}
	// hp <= m_stat.hp
	else {
		m_statInfo.hp = std::max((uint32)0, hp);
	}
}