#include "pch.h"
#include "Player.h"

#include "NPC.h"
#include "GameRoom.h"

Server::Contents::Player::Player(const TEAM_TYPE teamType)
	:Creature(GAME_OBJECT_TYPE::PLAYER, teamType)
{
	std::cout << std::format("Player! ID = {}", GetID()) << std::endl;
}

Server::Contents::Player::~Player()
{
	// std::cout << std::format("~Player! ID = {}", GetID()) << std::endl;
}

void Server::Contents::Player::Update(const float dt)
{
	GameObject::Update(dt);
}