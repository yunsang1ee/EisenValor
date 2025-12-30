#include "pch.h"
#include "NPC.h"

#include "GameRoom.h"
#include "SoldierStates.h"

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
	if(IsAlive()) {
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
		//GetGameRoom()->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
	}
}

bool Server::Contents::NPC::OnDamaged(std::shared_ptr<Creature> attacker, const int32 damaged, const float dt)
{
	const auto room = GetGameRoom();

	//if(FB_ENUMS::NPC_TYPE::NPC_TYPE_GENERAL == m_type) {
	//	// TODO: General OnDamaged
	//	return true;
	//}
	//else if(FB_ENUMS::NPC_TYPE::NPC_TYPE_SOLDIER == m_type) {
	//	const auto fsm = GetComponent<FSM>();
	//	const auto curStateType = fsm->GetCurState()->GetStateType();
	//	if(FB_ENUMS::SOLDIER_STATE_TYPE::SOLDIER_STATE_TYPE_ATTACK == curStateType) {
	//		room->AddEvent([fsm, dt]()
	//			{
	//				fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE::SOLDIER_STATE_TYPE_DAMAGED, dt);
	//				static_cast<Server::Contents::SoldierDamagedState*>(fsm->GetCurState())->SetStunTime(1.f);
	//			});
	//		return true;
	//	}
	//	else if(FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE == curStateType) {
	//		return false;
	//	}
	//	else {
	//		int curHp = GetHP();
	//		curHp -= damaged;
	//		if(curHp < 0) {
	//			SetAlive(false);
	//			// room->RemoveGameObject(shared_from_this());
	//			return true;
	//		}
	//		SetHp(curHp);
	//		room->AddEvent([fsm, dt]()
	//			{
	//				fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE::SOLDIER_STATE_TYPE_DAMAGED, dt);
	//				static_cast<Server::Contents::SoldierDamagedState*>(fsm->GetCurState())->SetStunTime(0.8f);
	//			});
	//		return true;
	//	}
	//}

	return false;
}
