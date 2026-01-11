#include "pch.h"
#include "Creature.h"

Server::Contents::Creature::Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type)
	:GameObject(teamType, type), m_alive{ true }
{
	SetCreature(true);
}

Server::Contents::Creature::~Creature()
{
}

void Server::Contents::Creature::SetHp(const uint32 hp) noexcept
{
	if(hp > m_statInfo.hp) {
		m_statInfo.hp = std::min(hp,  m_statInfo.maxHp);
	}
	else {
		m_statInfo.hp = std::max((uint32)0, hp);
	}
}

void Server::Contents::Creature::IncHP(const uint32 amount)
{
	const uint32 hp{ GetHP() + amount };
	m_statInfo.hp = std::min(hp, m_statInfo.maxHp);
}

void Server::Contents::Creature::DecHP(const uint32 amount)
{
	const uint32 hp{ GetHP() - amount };
	m_statInfo.hp = std::max(hp, (uint32)0);
}

void Server::Contents::Creature::IncStamina(const uint32 amount)
{
	const uint32 stamina{ GetStamina() + amount };
	m_statInfo.stamina = std::min(stamina, m_statInfo.maxStamina);
}

void Server::Contents::Creature::DecStamina(const uint32 amount)
{
	const uint32 stamina{ GetStamina() - amount };
	m_statInfo.stamina = std::max(stamina, (uint32)0);
}
