#include "pch.h"
#include "OccupationZone.h"

#include "GameWorld.h"

// #define PRINT_OCCUPATION_ZONE_LOG

// 게이지는 -100(BLUE 점령 완료) ~ 0(중립) ~ +100(RED 점령 완료) 범위를 갖는다.
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

	// 우세 팀이 바뀌면 재점령 유예시간을 초기화한다.
	// (소유 팀이 다시 우세해짐 / 양 팀 인원이 동수가 됨 / 점령지에 아무도 없어짐이 모두 여기에 해당)
	if(dominantTeamType != m_prevDominantTeamType) {
		m_prevDominantTeamType = dominantTeamType;
		m_graceAccDT = 0.f;
		BroadcastGauge(dominantTeamType);
	}

	// 점령이 완료된 거점은 점령지 내 인원과 무관하게 소유 팀에게 유지 점수를 지급한다.
	UpdateOwnerScore(dt);

	// 우세 팀이 없으면(동수/무인) 게이지는 현재 위치에서 정지한다.
	if(FB_ENUMS::TEAM_TYPE_NONE == dominantTeamType)
		return;

	if(false == CanMoveGauge(dominantTeamType, dt))
		return;

	const float prevGauge{ m_gauge };
	MoveGauge(dominantTeamType, dt);

	// 이미 자기 팀 방향으로 게이지가 가득 차 있으면 동기화할 변화가 없다.
	if(m_gauge == prevGauge)
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
	// 점령지는 높이를 무시한 원기둥 판정이다.
	const float distXZSq{ diff.x * diff.x + diff.z * diff.z };
	return distXZSq < m_rangeSq;
}

FB_ENUMS::TEAM_TYPE GameServer::Contents::OccupationZone::GetDominantTeamType() const
{
	auto const world{ GetOwner()->GetGameWorld() };
	const auto& groups{ world->GetGameObjectGroups() };

	int32 blueCount{};
	int32 redCount{};

	// 점령지 내부의 살아 있는 플레이어와 장수만 팀별로 집계한다.
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

	// 인원 차이는 점령 속도에 영향을 주지 않고 우세 여부만 결정한다.
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

bool GameServer::Contents::OccupationZone::CanMoveGauge(const FB_ENUMS::TEAM_TYPE dominantTeamType, const float dt)
{
	// 중립 상태의 점령지는 유예시간 없이 우세 팀이 생기는 즉시 게이지가 움직인다.
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED != m_stateType)
		return true;

	// 소유 팀이 우세하면 게이지를 자기 방향으로 되돌린다.
	if(dominantTeamType == m_ownerTeamType) {
		m_graceAccDT = 0.f;
		return true;
	}

	// 적 팀이 우세하면 유예시간 동안 계속 우세를 유지해야 게이지가 움직이기 시작한다.
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
	// 게이지가 중립 지점을 통과하는 순간 기존 소유 팀의 소유권이 해제된다.
	// 같은 팀이 계속 우세하면 유예 없이 그대로 자기 점령 방향으로 게이지가 이어서 움직인다.
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
	// 유지 점수 지급을 중단하고 타이머를 초기화한다.
	m_scoreAccDT = 0.f;
	// 중립 상태에는 유예시간이 적용되지 않는다.
	m_graceAccDT = 0.f;

	const auto owner{ GetOwner() };
	owner->GetGameWorld()->Broadcast(ServerPackets::Make_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(owner->GetID(), FB_ENUMS::TEAM_TYPE_NONE));
}

void GameServer::Contents::OccupationZone::OnOccupied(const FB_ENUMS::TEAM_TYPE teamType)
{
	// 소유권이 유지된 채 게이지만 되돌아온 경우에는 유지 점수 타이머를 건드리지 않는다.
	if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED == m_stateType && m_ownerTeamType == teamType)
		return;

	m_stateType = FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_OCCUPIED;
	m_ownerTeamType = teamType;
	// 새로운 팀이 점령했으므로 유지 점수 타이머를 새로 시작한다.
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
