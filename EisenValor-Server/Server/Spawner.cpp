#include "pch.h"
#include "Spawner.h"

#include "GameObject.h"
#include "GameWorld.h"
#include "Soldier.h"
#include "GameObjectFactory.h"

void GameServer::Contents::Spawner::Update(const float dt)
{
	m_accDT += dt;
	const auto owner = GetOwner();
	const auto world{ owner->GetGameWorld() };
		
	if(m_accDT >= SOLDIER_SPAWN_TIME.count()) {
		m_accDT = 0.f;

		for(int i = 0; i < SPAWN_NPC_COUNT; ++i) {
			SoldierTemplate t;
			t.id = world->GenerateID(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
			t.teamType = owner->GetTeamType();
			t.gameWorld = world;
			t.posInfo = owner->GetPosInfo();
			t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);

			auto soldier = GameServer::Contents::GameObjectFactory::CreateSoldier(t);
			world->AddGameObject(std::move(soldier));
		}
	}
}