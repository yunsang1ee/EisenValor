#include "pch.h"
#include "Creature.h"

Server::Contents::Creature::Creature(const GAME_OBJECT_TYPE type, const TEAM_TYPE teamType)
	:GameObject{type, teamType}
{
}

Server::Contents::Creature::~Creature()
{
}