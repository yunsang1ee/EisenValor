#include "pch.h"
#include "NPC.h"

#include "GameRoom.h"

Server::Contents::NPC::NPC(const FB_ENUMS::NPC_TYPE type, const FB_ENUMS::TEAM_TYPE team)
	:Creature{ FB_ENUMS::GAME_OBJECT_TYPE_NPC, team }, m_type{ type }
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
	GameObject::Update(dt);

	const uint32 id{ GetID() };
	const Vec3 pos{ GetPos() };
	const Vec3 rot{ GetRotation() };
	KinematicInfo kInfo{ pos, rot };
	const int32 hp{ GetHP() };
	uint8 state{};
	if(GetNpcType() == FB_ENUMS::NPC_TYPE_SOLDIER) 
		state = { GetComponent<FSM>()->GetCurState()->GetStateType() };

	auto pb = ServerPackets::Make_SC_NPC_INFO_PACKET(id, GetObjType(), GetTeamType(), GetNpcType(), kInfo, hp, state);
	GetGameRoom()->ExecuteAsyncronously(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
}
