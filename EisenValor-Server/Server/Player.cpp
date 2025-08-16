#include "pch.h"
#include "Player.h"

#include "NPC.h"
#include "GameRoom.h"

Server::Contents::Player::Player()
	:GameObject(GAME_OBJECT_TYPE::PLAYER)
{
}

void Server::Contents::Player::AddNpcs(std::shared_ptr<Server::Contents::NPC> npc)
{
	m_npcs.push_back(npc);
}

void Server::Contents::Player::Update(const float dt)
{
	std::cout << "Player Update!" << std::endl;
	m_kinematicInfo.position += m_kinematicInfo.velocity * dt;
	//auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(GetID(), m_kinematicInfo);
	//m_room.lock()->BroadcastInMatch(std::move(pb));	
}
