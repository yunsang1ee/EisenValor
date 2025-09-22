#include "pch.h"
#include "NPC.h"

#include "Player.h"
#include "GameRoom.h"

Server::Contents::NPC::NPC(const NPC_TYPE type, const TEAM_TYPE team)
	:Creature{ GAME_OBJECT_TYPE::NPC, team }, m_type{ type }
{
	static uint32 idGen{ 10000 };
	SetID(idGen);
	idGen++;
}

Server::Contents::NPC::~NPC()
{
	std::cout << "~Npc" << std::endl;
}
