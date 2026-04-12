#include "pch.h"
#include "HealZone.h"

#include "GameWorld.h"
#include "Creature.h"

GameServer::Contents::HealZone::HealZone(const float rangeSq, const int64 time, const uint32 healAmount)
	:m_rangeSq{ rangeSq }, m_time{ time }, m_healAmount{healAmount}
{
}

void GameServer::Contents::HealZone::Update(const float dt)
{
	if(!GetOwner()) return;
	auto gameWorld{ GetOwner()->GetGameWorld() };
	const auto& groups{ gameWorld->GetGameObjectGroups() };

	for(int i = 0; i < groups.size(); ++i) {
		if(i != etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) && i != etou8(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL))
			continue;

		for(const auto& [id, obj] : groups[i]) {
			if(!obj || false == obj->IsActive())
				continue;
			const auto distanceSq{ (GetOwner()->GetPosition() - obj->GetPosition()).LengthSquared() };
			if(distanceSq > m_rangeSq)
				continue;
			if(m_healObjects.contains(id))
				continue;
			m_healObjects.insert(std::make_pair(id, 0.f));
		}
	}

	auto iter{ m_healObjects.begin() };
	for(; iter != m_healObjects.end();) {
		const auto objID{ iter->first };
		const auto accHealTime{ iter->second };
		auto obj{ gameWorld->FindObjectByID(objID) };
		if(!obj || false == obj->IsActive()) {
			iter = m_healObjects.erase(iter);
			continue;
		}
		const auto distanceSq{ (GetOwner()->GetPosition() - obj->GetPosition()).LengthSquared() };
		if(distanceSq > m_rangeSq) {
			iter = m_healObjects.erase(iter);
			continue;
		}
		if(accHealTime >= m_time.count()) {
			std::static_pointer_cast<Creature>(obj)->IncHP(m_healAmount, true);
			iter->second = 0;
		}
		else {
			iter->second += dt;
			++iter;
		}
	}

}
