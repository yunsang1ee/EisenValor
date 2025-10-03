#include "pch.h"
#include "Player.h"

#include "NPC.h"
#include "GameRoom.h"

Server::Contents::Player::Player(const TEAM_TYPE teamType)
	:Creature(GAME_OBJECT_TYPE::PLAYER, teamType)
{
}

Server::Contents::Player::~Player()
{
}