#include "pch.h"
#include "Player.h"

#include "NPC.h"
#include "GameRoom.h"

Server::Contents::Player::Player(const FB_ENUMS::TEAM_TYPE teamType)
	:Creature(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER, teamType)
{
}

Server::Contents::Player::~Player()
{
}