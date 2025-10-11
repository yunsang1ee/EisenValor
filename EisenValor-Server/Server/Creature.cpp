#include "pch.h"
#include "Creature.h"

Server::Contents::Creature::Creature(const FB_ENUMS::GAME_OBJECT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType)
	:GameObject{type, teamType}
{
}

Server::Contents::Creature::~Creature()
{
}