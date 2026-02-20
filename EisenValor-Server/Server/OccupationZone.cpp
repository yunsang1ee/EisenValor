#include "pch.h"
#include "OccupationZone.h"

#include "GameWorld.h"

Server::Contents::OccupationZone::OccupationZone(const float rangeSq, const int64 time)
	:m_accDT{}, m_stateType{FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED}, m_rangeSq{rangeSq}, m_time{time}
{
}

void Server::Contents::OccupationZone::Update(const float dt)
{
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED == m_stateType) 
		return;

	auto const owner{ GetOwner() };
	auto const ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };
	const auto& groups{ world->GetGameObjectGroups() };
	
	for(int i = 0; i < groups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) 
			continue;

		for(const auto& [id, o] : groups[i]) {
			if(owner->GetTeamType() == o->GetTeamType()) continue;

			const auto& otherPos{ o->GetPos() };
			const auto& distToOtherSq{ (otherPos - ownerPos).LengthSquared() };

			if(distToOtherSq <= m_rangeSq) {
				m_accDT += dt;
			}

			if(m_accDT >= m_time.count()) {
				m_stateType = FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED;
				auto pb{ ServerPackets::Make_SC_UPDATE_STATE_PACKET(owner->GetID(), m_stateType) };
				world->Broadcast(std::move(pb));
				return;
			}
		}
	}
}
