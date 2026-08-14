#include "pch.h"
#include "OccupationZone.h"

#include "GameWorld.h"

// #define PRINT_OCCUPATION_ZONE_LOG

namespace {
	constexpr float OCCUPATION_GAUGE_MAX{ 100.f };
	constexpr float OCCUPATION_GAUGE_NEUTRAL{ 0.f };
}

GameServer::Contents::OccupationZone::OccupationZone(const float rangeSq, const int64 scoreTime, const float recaptureGraceSec)
	: m_stateType{ FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED }
	, m_ownerTeamType{ FB_ENUMS::TEAM_TYPE_NONE }
	, m_rangeSq{ rangeSq }
	, m_scoreTime{ scoreTime }
	, m_recaptureGraceSec{ recaptureGraceSec }
	, m_rateOfGaugeIncrease{ 5.f }
	, m_gauge{}
	, m_prevDominantTeamType{ FB_ENUMS::TEAM_TYPE_NONE }
	, m_lastSentGauge{}
	, m_syncAccDT{}
	, m_scoreAccDT{}
	, m_graceAccDT{}
{
}

void GameServer::Contents::OccupationZone::Update(const float dt)
{
	const auto dominantTeamType{ GetDominantTeamType() };

	if(dominantTeamType != m_prevDominantTeamType) {
		m_prevDominantTeamType = dominantTeamType;
		m_graceAccDT = 0.f;
		BroadcastGauge(dominantTeamType);
	}

	const auto prevStateType{ m_stateType };
	const auto prevOwnerTeamType{ m_ownerTeamType };

	const bool gaugeMoved{ UpdateGauge(dominantTeamType, dt) };

	if(prevStateType == m_stateType && prevOwnerTeamType == m_ownerTeamType)
		UpdateOwnerScore(dt);

	if(false == gaugeMoved)
		return;

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

bool GameServer::Contents::OccupationZone::IsInOccupationZone(const Vec3& pos) const
{
	const auto owner{ GetOwner() };
	const Vec3 diff{ pos - owner->GetPosition() };
	const float distXZSq{ diff.x * diff.x + diff.z * diff.z };
	return distXZSq < m_rangeSq;
}

FB_ENUMS::TEAM_TYPE GameServer::Contents::OccupationZone::GetDominantTeamType() const
{
	auto const world{ GetOwner()->GetGameWorld() };
	const auto& groups{ world->GetGameObjectGroups() };

	int32 blueCount{};
	int32 redCount{};

	for(int i = 0; i < groups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
			continue;

		for(const auto& [id, o] : groups[i]) {
			if(false == IsValidObj(o)) continue;

			if(false == IsInOccupationZone(o->GetPosition())) continue;

			if(o->GetTeamType() == FB_ENUMS::TEAM_TYPE_BLUE) {
				blueCount++;
			}
			else if(o->GetTeamType() == FB_ENUMS::TEAM_TYPE_RED) {
				redCount++;
			}
		}
	}

	if(blueCount > redCount) return FB_ENUMS::TEAM_TYPE_BLUE;
	if(redCount > blueCount) return FB_ENUMS::TEAM_TYPE_RED;
	return FB_ENUMS::TEAM_TYPE_NONE;
}

void GameServer::Contents::OccupationZone::UpdateOwnerScore(const float dt)
{
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED != m_stateType)
		return;

	if(FB_ENUMS::TEAM_TYPE_NONE == m_ownerTeamType)
		return;

	const float scoreTimeSec{ static_cast<float>(m_scoreTime.count()) };

	m_scoreAccDT += dt;
	if(m_scoreAccDT < scoreTimeSec)
		return;

	m_scoreAccDT -= scoreTimeSec;

	auto const zoneData{ MANAGER(GameServer::Contents::MapDataManager)->GetOccupationZone("Map", GetName()) };
	if(!zoneData)
		return;

	GetOwner()->GetGameWorld()->AddScore(m_ownerTeamType, zoneData->scorePerTenSec);
#ifdef PRINT_OCCUPATION_ZONE_LOG
	std::cout << "Score Added to " << (m_ownerTeamType == FB_ENUMS::TEAM_TYPE_BLUE ? "Blue Team!" : "Red Team!") << std::endl;
#endif
}

bool GameServer::Contents::OccupationZone::UpdateGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType, const float dt)
{
	if(FB_ENUMS::TEAM_TYPE_NONE == dominantTeamType)
		return false;

	if(false == CanMoveGauge(dominantTeamType, dt))
		return false;

	const float prevGauge{ m_gauge };
	MoveGauge(dominantTeamType, dt);

	return m_gauge != prevGauge;
}

bool GameServer::Contents::OccupationZone::CanMoveGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType, const float dt)
{
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED != m_stateType)
		return true;

	if(dominantTeamType == m_ownerTeamType) {
		m_graceAccDT = 0.f;
		return true;
	}

	m_graceAccDT += dt;
	return m_graceAccDT >= m_recaptureGraceSec;
}

void GameServer::Contents::OccupationZone::MoveGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType, const float dt)
{
	const float prevGauge{ m_gauge };

	if(FB_ENUMS::TEAM_TYPE_BLUE == dominantTeamType) {
		m_gauge -= m_rateOfGaugeIncrease * dt;
	}
	else if(FB_ENUMS::TEAM_TYPE_RED == dominantTeamType) {
		m_gauge += m_rateOfGaugeIncrease * dt;
	}

	m_gauge = std::clamp(m_gauge, -OCCUPATION_GAUGE_MAX, OCCUPATION_GAUGE_MAX);
	CheckOccupationState(prevGauge, m_gauge);
}

void GameServer::Contents::OccupationZone::CheckOccupationState(const float prev, const float curr)
{
	const bool crossedNeutral{
		(prev > OCCUPATION_GAUGE_NEUTRAL && curr <= OCCUPATION_GAUGE_NEUTRAL) ||
		(prev < OCCUPATION_GAUGE_NEUTRAL && curr >= OCCUPATION_GAUGE_NEUTRAL) };
	if(crossedNeutral) {
		OnNeutralized();
	}

	if(prev < OCCUPATION_GAUGE_MAX && curr >= OCCUPATION_GAUGE_MAX) {
		OnOccupied(FB_ENUMS::TEAM_TYPE_RED);
	}
	else if(prev > -OCCUPATION_GAUGE_MAX && curr <= -OCCUPATION_GAUGE_MAX) {
		OnOccupied(FB_ENUMS::TEAM_TYPE_BLUE);
	}
}

void GameServer::Contents::OccupationZone::OnNeutralized()
{
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == m_stateType && FB_ENUMS::TEAM_TYPE_NONE == m_ownerTeamType)
		return;

#ifdef PRINT_OCCUPATION_ZONE_LOG
	std::cout << "Occupation Zone Neutralized. (Previous Owner: " << (m_ownerTeamType == FB_ENUMS::TEAM_TYPE_BLUE ? "Blue Team" : "Red Team") << ")" << std::endl;
#endif

	m_stateType = FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED;
	m_ownerTeamType = FB_ENUMS::TEAM_TYPE_NONE;
	m_scoreAccDT = 0.f;
	m_graceAccDT = 0.f;

	const auto owner{ GetOwner() };
	owner->GetGameWorld()->Broadcast(ServerPackets::Make_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(owner->GetID(), FB_ENUMS::TEAM_TYPE_NONE));
}

void GameServer::Contents::OccupationZone::OnOccupied(const FB_ENUMS::TEAM_TYPE teamType)
{
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED == m_stateType && m_ownerTeamType == teamType)
		return;

	m_stateType = FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED;
	m_ownerTeamType = teamType;
	m_scoreAccDT = 0.f;
	m_graceAccDT = 0.f;

	const auto owner{ GetOwner() };
	owner->GetGameWorld()->Broadcast(ServerPackets::Make_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(owner->GetID(), teamType));

#ifdef PRINT_OCCUPATION_ZONE_LOG
	std::cout << "Occupation Zone Captured by " << (teamType == FB_ENUMS::TEAM_TYPE_BLUE ? "Blue Team!" : "Red Team!") << std::endl;
#endif
}

void GameServer::Contents::OccupationZone::BroadcastGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType)
{
	const auto owner{ GetOwner() };
	auto pb = ServerPackets::Make_SC_OCCUPATION_ZONE_GAUGE_PACKET(owner->GetID(), m_gauge, dominantTeamType);

	owner->GetGameWorld()->Broadcast(std::move(pb));

	m_lastSentGauge = m_gauge;

#ifdef PRINT_OCCUPATION_ZONE_LOG
	std::cout << "Occupation Zone Gauge Updated: " << m_gauge << " (Dominant Team: " << (dominantTeamType == FB_ENUMS::TEAM_TYPE_BLUE ? "Blue" : (dominantTeamType == FB_ENUMS::TEAM_TYPE_RED ? "Red" : "None")) << ")" << std::endl;
#endif
}
