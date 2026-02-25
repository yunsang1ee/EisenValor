#include "pch.h"
#include "Spawner.h"

#include "GameObject.h"
#include "GameWorld.h"
#include "Soldier.h"

void Server::Contents::Spawner::Update(const float dt)
{
	m_accDT += dt;
	const auto owner = GetOwner();
	const auto world{ owner->GetGameWorld() };

	if(m_accDT >= SOLDIER_SPAWN_TIME.count()) {
		m_accDT = 0.f;
		
		for(int i = 0; i < SPAWN_NPC_COUNT; ++i) {
			SoldierTemplate s;

			// TODO: Spanwer Update

			// auto soldier = Server::Contents::GameObjectFactory::CreateSoldier(s);
			// world->AddGameObject(std::move(soldier));
		}
	}
}