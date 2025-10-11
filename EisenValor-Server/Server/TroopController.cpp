#include "pch.h"
#include "TroopController.h"

#include "NPC.h"
#include "Player.h"
#include "SoldierStates.h"

void Server::Contents::TroopController::Init()
{
	auto lineFormation = std::make_unique<LineFormation>();
	lineFormation->m_controller = this;
	lineFormation->m_formationType = TROOP_FORMATION_TYPE::LINE;
	lineFormation->Init();
	m_formations.try_emplace(TROOP_FORMATION_TYPE::LINE, std::move(lineFormation));

	auto vShapeFormation = std::make_unique<VShapeFormation>();
	vShapeFormation->m_controller = this;
	vShapeFormation->m_formationType = TROOP_FORMATION_TYPE::VSHAPE;
	vShapeFormation->Init();
	m_formations.try_emplace(TROOP_FORMATION_TYPE::VSHAPE, std::move(vShapeFormation));

	auto circleFormation = std::make_unique<CircleFormation>();
	circleFormation->m_controller = this;
	circleFormation->m_formationType = TROOP_FORMATION_TYPE::CIRCLE;
	circleFormation->Init();
	m_formations.try_emplace(TROOP_FORMATION_TYPE::CIRCLE, std::move(circleFormation));

	m_curFormation = m_formations[TROOP_FORMATION_TYPE::LINE].get();
}

void Server::Contents::TroopController::AddSoldier(std::shared_ptr<NPC> soldier)
{
	const uint32 id{ soldier->GetID() };
	SoldierData data{ Vec3{0.f, 0.f, 0.f}, std::move(soldier)};
	m_soldiers.try_emplace(id, data);
}

void Server::Contents::TroopController::RemoveSoldier(std::shared_ptr<NPC> soldier)
{
	const uint32 id{ soldier->GetID() };
	m_soldiers.erase(id);
}

void Server::Contents::TroopController::Update(const float dt)
{
	if(!m_curFormation)
		return;

	// 1. 플레이어 위치 변화 추적
	const Vec3& curOwnerPos = GetOwner()->GetPos();
	Vec3 delta = curOwnerPos - m_prevOwnerPos;
	const float lenSq = delta.LengthSquared();
	m_prevOwnerPos = curOwnerPos;
	if(lenSq > 0.0001f) {
		// 플레이어 이동량만큼 대열 중심도 이동
		m_curFormation->m_centerPos += delta;
		
		// 2. 현재 formation에 맞춰 병사 위치 갱신
		m_curFormation->Arrange(m_soldiers);
	}
	else {
		return;
	}
}

void Server::Contents::TroopController::SetFormation(const TROOP_FORMATION_TYPE type) noexcept
{
	auto iter = m_formations.find(type);
	if(iter != m_formations.end()) {
		m_curFormation = iter->second.get();
		m_curFormation->Arrange(m_soldiers);
	}
}

void Server::Contents::TroopController::Arrange()
{
	m_curFormation->Arrange(m_soldiers);
}

void Server::Contents::TroopController::SetTargetPos(const Vec3& targetPos)
{
	m_curFormation->m_centerPos = targetPos;
	m_curFormation->Arrange(m_soldiers);
}