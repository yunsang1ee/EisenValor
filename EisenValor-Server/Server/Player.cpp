#include "pch.h"
#include "Player.h"

#include "NPC.h"

Server::Contents::Player::Player()
	:GameObject(GAME_OBJECT_TYPE::PLAYER)
{
}

void Server::Contents::Player::AddNpcs(std::shared_ptr<Server::Contents::NPC> npc)
{
	m_npcs.push_back(npc);
	npc->ExecuteAsyncronously(&Server::Contents::NPC::Update);
}
