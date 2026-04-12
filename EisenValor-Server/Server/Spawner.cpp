#include "pch.h"
#include "Spawner.h"

#include "GameObject.h"
#include "GameWorld.h"
#include "Soldier.h"
#include "GameObjectFactory.h"

GameServer::Contents::Spawner::Spawner(const Vec3& destPos, const uint32 spawnTimeSec, const uint32 spawnCount)
	: m_accDT{}, m_soldierDestPos{ destPos }, m_spawnTimeSec{ spawnTimeSec }, m_spawnCount{ spawnCount }
{
}

void GameServer::Contents::Spawner::Update(const float dt)
{
	m_accDT += dt;
	const auto owner = GetOwner();
	const auto world{ owner->GetGameWorld() };
		
	if(m_accDT >= m_spawnTimeSec.count()) {
		m_accDT -= m_spawnTimeSec.count();

		for(uint32 i = 0; i < m_spawnCount; ++i) {
			SoldierTemplate t;
			t.id = world->GenerateID(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
			t.teamType = owner->GetTeamType();
			t.gameWorld = world;
			t.transform = owner->GetTransform();
			t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
			t.destPos = m_soldierDestPos;

			auto soldier = GameServer::Contents::GameObjectFactory::CreateSoldier(t);
			world->AddGameObject(std::move(soldier));
		}
	}
}