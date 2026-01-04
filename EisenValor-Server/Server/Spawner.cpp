#include "pch.h"
#include "Spawner.h"

#include "GameObject.h"
#include "GameRoom.h"

void Server::Contents::Spawner::Update(const float dt)
{
	//m_accDT += dt;
	//const auto owner = GetOwner();
	//auto gameRoom = owner->GetGameRoom();
	//auto& team= gameRoom->GetTeam(GetOwner()->GetTeamType());

	//if(m_accDT >= SOLDIER_SPAWN_TIME.count()) {
	//	m_accDT = 0.f;
	//	
	//	for(int i = 0; i < SPAWN_NPC_COUNT; ++i) {
	//		SoldierTemplate s;
	//		s.npcType = FB_ENUMS::NPC_TYPE_SOLDIER;
	//		s.teamType = owner->GetTeamType();
	//		s.stat = StatInfo{ 100, 10, 100 };
	//		s.enemyDetectionRange = 100.f;
	//		s.combatRange = 0.3f;
	//		s.attackCycleTime = 1s;

	//		switch(s.teamType) {
	//			case FB_ENUMS::TEAM_TYPE_OFFENSE:
	//			{
	//				s.pos = Vec3{ 0.f + i * 0.5f, 0.f, -5.f };
	//				break;
	//			}
	//			case FB_ENUMS::TEAM_TYPE_DEFENSE:
	//			{
	//				s.pos = Vec3{ 0.f + i * 0.5f, 0.f,5.f };
	//				s.rot = Vec3{ 0.f, 135.f, 0.f };
	//				break;
	//			}
	//			default:
	//				break;
	//		}
	//		auto soldier = Server::Contents::GameObjectFactory::CreateSoldier(s);
	//		// gameRoom->AddGameObject(std::move(soldier));
	//	}
	//}
}