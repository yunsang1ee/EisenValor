#include "pch.h"
#include "Team.h"

#include "Player.h"
#include "NPC.h"
#include "GameRoom.h"
#include "GameObjectFactory.h"

Server::Contents::Team::Team(const FB_ENUMS::TEAM_TYPE type)
	:m_type{ type }
{

}

void Server::Contents::Team::Init(std::shared_ptr<GameRoom> room)
{
	m_room = room;

	{
		SpanwerTemplate spawner;
		spawner.objType = FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER;
		spawner.teamType = m_type;
		if(m_type == FB_ENUMS::TEAM_TYPE_RED) {
			spawner.pos = Vec3{ 0.f, 0.f, 7.f };
		}
		else
			spawner.pos = Vec3{ 0.f, 0.f, -7.f };

		auto spawnObj = Server::Contents::GameObjectFactory::CreateSpawnObj(spawner);
		AddObject(std::move(spawnObj));
	}

}

void Server::Contents::Team::AddObject(std::shared_ptr<GameObject> object)
{
	object->SetRoom(m_room);

	const uint32 id{ object->GetID() };
	const uint8 type{ etou8(object->GetObjType()) };

	const uint16 genID = object->GetID();
	const Vec3 pos{ object->GetPos() };
	const Vec3 rot{ object->GetRotation() };
	const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };

	if(object->GetObjType() == FB_ENUMS::GAME_OBJECT_TYPE_NPC) {
		const auto npc = std::static_pointer_cast<NPC>(object);
		auto pb = ServerPackets::Make_SC_ADD_NPC_PACKET(genID, object->GetObjType(), object->GetTeamType(), npc->GetNpcType(), kInfo, npc->GetHP());
		m_room->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
	}
	else {
		if(object->GetObjType() == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) {
			auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(genID, object->GetObjType(), object->GetTeamType(), kInfo, std::static_pointer_cast<Player>(object)->GetHP());
			m_room->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
		}
		else {
			auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(genID, object->GetObjType(), object->GetTeamType(), kInfo, 0);
			m_room->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
		}
	}

	if(m_objectGroups[type].find(id) == m_objectGroups[type].end())
		m_objectGroups[type].try_emplace(id, std::move(object));
}

void Server::Contents::Team::RemoveObject(std::shared_ptr<GameObject> object)
{
	const uint32 id{ object->GetID() };
	const FB_ENUMS::GAME_OBJECT_TYPE objType{ object->GetObjType() };
	auto pb = ServerPackets::Make_SC_REMOVE_OBJ(id);
	m_room->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
	if(m_objectGroups[etou8(objType)].find(id) != m_objectGroups[etou8(objType)].end()) {
		m_objectGroups[etou8(objType)].erase(id);
		std::cout << std::format("ID: {} 게임에서 삭제!", id) << std::endl;;
	}
}

std::shared_ptr<Server::Contents::GameObject> Server::Contents::Team::GetObj(const uint32 id)
{
	for(auto& group : m_objectGroups) {
		auto iter = group.find(id);
		if(iter != group.end()) return iter->second;
	}
	return nullptr;
}
