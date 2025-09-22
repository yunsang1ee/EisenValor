#include "pch.h"
#include "Creature.h"

Server::Contents::Creature::Creature(const GAME_OBJECT_TYPE type, const TEAM_TYPE team)
	:GameObject{type, team}
{
	// std::cout << std::format("CREATURE! ID = {}", GetID()) << std::endl;
}

Server::Contents::Creature::~Creature()
{
	std::cout << std::format("~CREATURE! ID = {}", GetID()) << std::endl;
}
