#include "pch.h"
#include "Soldier.h"

Server::Contents::Soldier::Soldier(const FB_ENUMS::TEAM_TYPE teamType)
	:Creature{teamType, FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER}
{
}

Server::Contents::Soldier::~Soldier()
{
}
