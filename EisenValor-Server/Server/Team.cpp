#include "pch.h"
#include "Team.h"

#include "Player.h"
#include "NPC.h"
#include "GameRoom.h"

Server::Contents::Team::Team(const TEAM_TYPE type)
	:m_type{ type }
{

}

void Server::Contents::Team::AddObject(std::shared_ptr<GameObject> object)
{
	const uint32 id{ object->GetID() };
	const uint8 type{ etou8(object->GetObjType()) };

	if(m_objects[type].find(id) == m_objects[type].end())
		m_objects[type].try_emplace(id, object);

	const uint16 genID = object->GetID();
	const Vec3 pos{ object->GetPos() };
	const Vec3 rot{ object->GetRotation() };
	const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };

	auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(genID, static_cast<uint8>(object->GetObjType()), object->GetTeamType(), kInfo);
	object->GetGameRoom()->ExecuteAsyncronously(&Server::Contents::GameRoom::Broadcast, std::move(pb));
}

void Server::Contents::Team::RemoveObject(std::shared_ptr<GameObject> object)
{
	const uint32 id{ object->GetID() };
	const GAME_OBJECT_TYPE objType{ object->GetObjType() };
	auto pb = ClientPacketHandler::Make_SC_REMOVE_OBJ(id);
	object->GetGameRoom()->ExecuteAsyncronously(&Server::Contents::GameRoom::Broadcast, std::move(pb));
	if(m_objects[etou8(objType)].find(id) != m_objects[etou8(objType)].end())
		m_objects[etou8(objType)].erase(id);
}

//void Server::Contents::Team::AddPlayer(std::shared_ptr<Player> player)
//{
//	const uint32 id{ player->GetID() };
//	m_players.try_emplace(id, player);
//
//}
//
//void Server::Contents::Team::AddNpc(std::shared_ptr<NPC> npc)
//{
//	const uint32 id{ npc->GetID() };
//	m_players.try_emplace(id, npc);
//}