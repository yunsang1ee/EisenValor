#include "pch.h"
#include "Team.h"

#include "Player.h"
#include "NPC.h"
#include "GameRoom.h"
#include "GameObjectFactory.h"

Server::Contents::Team::Team(const TEAM_TYPE type)
	:m_type{ type }
{

}

void Server::Contents::Team::Init(std::shared_ptr<GameRoom> room)
{
	m_room = room;

	//static Vec3 offset{ 1.f, 0.f, 1.f };
	//GeneralTemplate g;
	//g.npcType = NPC_TYPE::GENERAL;
	//g.objType = GAME_OBJECT_TYPE::NPC;
	//g.pos = offset;
	//g.rot = Vec3{ 0.f, 0.f, 0.f };
	//offset.x += 1.f;
	//offset.z += 1.f;
	//g.teamType = m_type;
	//g.stat.hp = 100;
	//
	//auto general = Server::Contents::GameObjectFactory::CreateGeneral(g);
	//AddObject(std::move(general));
	
	for(int i = 0; i < 1; ++i) {
		SoldierTemplate s;
		s.npcType = NPC_TYPE::SOLDIER;
		s.objType = GAME_OBJECT_TYPE::NPC;
		s.teamType = m_type;
		s.stat = StatInfo{ 100, 10, 100 };

		switch(m_type) {
			case TEAM_TYPE::BLUE:
			{
				s.pos = Vec3{ 0.f + i * 0.5f, 0.f, -5.f };
				break;
			}
			case TEAM_TYPE::RED:
			{
				s.pos = Vec3{ 0.f +  i * 0.5f, 0.f, 5.f };
				break;
			}
			default:
				break;
		}

		auto soldier = Server::Contents::GameObjectFactory::CreateSoldier(s);
		AddObject(std::move(soldier));
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

	auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(genID, static_cast<uint8>(object->GetObjType()), object->GetTeamType(), kInfo, std::static_pointer_cast<NPC>(object)->GetNpcType());
	m_room->ExecuteAsyncronously(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));

	if(m_objects[type].find(id) == m_objects[type].end())
		m_objects[type].try_emplace(id, std::move(object));
}

void Server::Contents::Team::RemoveObject(std::shared_ptr<GameObject> object)
{
	const uint32 id{ object->GetID() };
	const GAME_OBJECT_TYPE objType{ object->GetObjType() };
	auto pb = ClientPacketHandler::Make_SC_REMOVE_OBJ(id);
	m_room->ExecuteAsyncronously(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
	if(m_objects[etou8(objType)].find(id) != m_objects[etou8(objType)].end()) {
		m_objects[etou8(objType)].erase(id);
		std::cout << std::format("ID: {} 게임에서 삭제!", id) << std::endl;;
	}
}

std::shared_ptr<Server::Contents::GameObject> Server::Contents::Team::GetObj(const uint32 id)
{
	for(auto& group : m_objects) {
		auto iter = group.find(id);
		if(iter != group.end()) return iter->second;
	}
	return nullptr;
}
