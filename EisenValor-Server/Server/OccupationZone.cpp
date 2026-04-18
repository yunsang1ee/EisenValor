#include "pch.h"
#include "OccupationZone.h"

#include "GameWorld.h"

GameServer::Contents::OccupationZone::OccupationZone(const float rangeSq, const int64 scoreTime)
	: m_stateType{ FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED }
	, m_rangeSq{ rangeSq }
	, m_scoreTime{ scoreTime }
	, m_rateOfGaugeIncrease{ 5.f }
	, m_gauge{}
	, m_prevDominantTeamType{ FB_ENUMS::TEAM_TYPE_NONE }
	, m_lastSentGauge{}
	, m_syncAccDT{}
	, m_dominantAccDT{}
{
}

void GameServer::Contents::OccupationZone::Update(const float dt)
{
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED == m_stateType)
		return;

	const auto dominantTeamType{ GetDominantTeamType() };

	if(dominantTeamType != m_prevDominantTeamType) {
		m_prevDominantTeamType = dominantTeamType;
		m_dominantAccDT = 0.f;
		BroadcastGauge(dominantTeamType);
	}

	if(FB_ENUMS::TEAM_TYPE_NONE == dominantTeamType)
		return;

	m_dominantAccDT += dt;
	if(m_dominantAccDT >= static_cast<float>(m_scoreTime.count())) {
		m_dominantAccDT = 0.f;
		const auto owner{ GetOwner() };
		owner->GetGameWorld()->AddScore(dominantTeamType, MANAGER(GameServer::Contents::MapDataManager)->GetOccupationZone("Map", GetName())->scorePerTenSec);
		std::cout << "Score Added to " << (dominantTeamType == FB_ENUMS::TEAM_TYPE_BLUE ? "Blue Team!" : "Red Team!") << std::endl;
	}

	const float prevGauge{ m_gauge };
	if(FB_ENUMS::TEAM_TYPE_BLUE == dominantTeamType) {
		m_gauge -= m_rateOfGaugeIncrease * dt;
		// std::cout << "Blue Gauge: " << m_gauge << std::endl;
	}
	else if(FB_ENUMS::TEAM_TYPE_RED == dominantTeamType) {
		m_gauge += m_rateOfGaugeIncrease * dt;
		// std::cout << "Red Gauge: " << m_gauge << std::endl;
	}

	m_gauge = std::clamp(m_gauge, -100.0f, 100.0f);
	CheckOccupationState(prevGauge, m_gauge);

	m_syncAccDT += dt;
	if(m_syncAccDT >= 0.5f) {
		m_syncAccDT = 0.f;
		BroadcastGauge(dominantTeamType);
		return;
	}
	if(std::abs(m_gauge - m_lastSentGauge) >= 1.0f) {
		BroadcastGauge(dominantTeamType);
	}
}

FB_ENUMS::TEAM_TYPE GameServer::Contents::OccupationZone::GetDominantTeamType()
{
	const auto owner{ GetOwner() };
	const auto ownerPos{ owner->GetPosition() };
	auto const world{ owner->GetGameWorld() };
	const auto& groups{ world->GetGameObjectGroups() };

	int32 blueCount{};
	int32 redCount{};

	for(int i = 0; i < groups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
			continue;

		for(const auto& [id, o] : groups[i]) {
			auto obj{ o.get() };

			if(false == IsValidObj(o)) continue;

			const float distSq{ (obj->GetPosition() - ownerPos).LengthSquared() };
			if(distSq < m_rangeSq) {
				if(obj->GetTeamType() == FB_ENUMS::TEAM_TYPE_BLUE) {
					blueCount++;
				}
				else if(obj->GetTeamType() == FB_ENUMS::TEAM_TYPE_RED) {
					redCount++;
				}
			}
		}
	}

	if(blueCount > redCount) return FB_ENUMS::TEAM_TYPE_BLUE;
	if(redCount > blueCount) return FB_ENUMS::TEAM_TYPE_RED;
	return FB_ENUMS::TEAM_TYPE_NONE;
}

void GameServer::Contents::OccupationZone::CheckOccupationState(const float prev, const float curr)
{
	const auto [occupied, team] = [&]() -> std::pair<bool, FB_ENUMS::TEAM_TYPE>
		{
			if(prev < 100.f && curr >= 100.f)  return { true, FB_ENUMS::TEAM_TYPE_RED };
			if(prev > -100.f && curr <= -100.f) return { true, FB_ENUMS::TEAM_TYPE_BLUE };
			return { false, {} };
		}();

	if(occupied) {
		m_stateType = FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED;
		const auto owner = GetOwner();
		const auto world = owner->GetGameWorld();
		world->Broadcast(ServerPackets::Make_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(owner->GetID(), team));
		world->AddScore(team, MANAGER(GameServer::Contents::MapDataManager)->GetOccupationZone("Map", GetName())->occupationScore);
		return;
	}
	
	if((prev > 0.f && curr <= 0.f) || (prev < 0.f && curr >= 0.f)) {
		m_stateType = FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED;
	}
}

void GameServer::Contents::OccupationZone::BroadcastGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType)
{
	const auto owner{ GetOwner() };
	auto pb = ServerPackets::Make_SC_OCCUPATION_ZONE_GAUGE_PACKET(owner->GetID(), m_gauge, dominantTeamType);
	
	owner->GetGameWorld()->Broadcast(std::move(pb));

	m_lastSentGauge = m_gauge;

	std::cout << "Occupation Zone Gauge Updated: " << m_gauge << " (Dominant Team: " << (dominantTeamType == FB_ENUMS::TEAM_TYPE_BLUE ? "Blue" : (dominantTeamType == FB_ENUMS::TEAM_TYPE_RED ? "Red" : "None")) << ")" << std::endl;
}