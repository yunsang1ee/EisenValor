#include "pch.h"
#include "NPC.h"

#include "GameRoom.h"

Server::Contents::NPC::NPC(const NPC_TYPE type, const TEAM_TYPE team)
	:Creature{ GAME_OBJECT_TYPE::NPC, team }, m_type{ type }
{
	static uint32 idGen{ 10000 };
	SetID(idGen);
	idGen++;
	std::cout << std::format("NPC! ID = {}", GetID()) << std::endl;
}

Server::Contents::NPC::~NPC()
{
	std::cout << std::format("~NPC! ID = {}", GetID()) << std::endl;
}

void Server::Contents::NPC::Update(const float dt)
{
	// TODO: 여기서 NPC_INFO 보내주기
	GameObject::Update(dt);

	const uint32 id{ GetID() };
	const uint8 objType{ etou8(GetObjType()) };
	const uint8 teamType{ etou8(GetTeamType()) };
	const uint8 npcType{ etou8(GetNpcType()) };
	const Vec3 pos{ GetPos() };
	const Vec3 rot{ GetRotation() };
	KinematicInfo kInfo{ pos, rot };
	const int32 hp{ GetHP() };
	uint8 state{};
	if(npcType == etou8(NPC_TYPE::SOLDIER)) {
		state = { GetComponent<FSM>()->GetCurState()->GetStateType() };
	}
	auto pb = ServerPackets::Make_SC_NPC_INFO_PACKET(id, objType, teamType, npcType, kInfo, hp, state);
	GetGameRoom()->ExecuteAsyncronously(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
	// std::cout << std::format("ID: {} HP: {}, Update!", id, hp) << std::endl;
}
