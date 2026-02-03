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
	if(hp > m_statInfo.currentHP) {
		m_statInfo.currentHP = std::min(hp,  m_statInfo.maxHP);
	}
	else {
		m_statInfo.currentHP = std::max((uint32)0, hp);
	}
}

void Server::Contents::Creature::IncHP(const uint32 amount)
{
	const uint32 hp{ GetHP() + amount };
	m_statInfo.currentHP = std::min(hp, m_statInfo.maxHP);
}

void Server::Contents::Creature::DecHP(const uint32 amount)
{
	m_statInfo.currentHP = std::max(static_cast<int32>(GetHP()) - static_cast<int32>(amount), 0);
	if(m_statInfo.currentHP == 0 && m_alive) {
		m_alive = false;
		OnDeath();
	}
}

void Server::Contents::Creature::IncStamina(const uint32 amount)
{
	const uint32 stamina{ GetStamina() + amount };
	m_statInfo.currentStamina = std::min(stamina, m_statInfo.maxStamina);
}

void Server::Contents::Creature::DecStamina(const uint32 amount)
{
	const uint32 stamina{ GetStamina() - amount };
	m_statInfo.currentStamina = std::max(static_cast<int32>(GetStamina()) - static_cast<int32>(amount), 0);
}

void Server::Contents::Creature::IncRespawnTime()
{
	m_statInfo.respawnTimeSec += m_statInfo.respawnTimeIncAmount;
}

