#include "pch.h"
#include "TroopController.h"

#include "NPC.h"
#include "Player.h"
#include "SoldierState.h"

void Server::Contents::TroopController::Init()
{
	auto lineFormation = std::make_unique<LineFormation>();
	lineFormation->m_controller = this;
	m_formations.try_emplace(TROOP_FORMATION_TYPE::LINE, std::move(lineFormation));
}

void Server::Contents::TroopController::AddSoldier(std::shared_ptr<NPC> soldier)
{
	const uint32 id{ soldier->GetID() };
	m_soldiers.try_emplace(id, soldier);
}

void Server::Contents::TroopController::RemoveSoldier(std::shared_ptr<NPC> soldier)
{
	const uint32 id{ soldier->GetID() };
	m_soldiers.erase(id);
}

void Server::Contents::TroopController::Update(const float dt)
{
	// TODO: 여기선 뭐해야 하냐?
	// TOOD: 여기서 할 게 있나?
}

void Server::Contents::TroopController::SetFormation(const TROOP_FORMATION_TYPE type) noexcept
{
	m_curFormation = m_formations.find(type)->second.get();
	m_curFormation->Arrange(m_soldiers);
}

void Server::Contents::LineFormation::Arrange(std::unordered_map<uint32, std::shared_ptr<NPC>>& soldiers) noexcept
{
	const int kMaxSoldierCount = soldiers.size();
	constexpr int kRowSize = 5;
	constexpr float kSpacing = 3.f;
	int currentCount{};

	int soldiersToSummon = kMaxSoldierCount;
	
	const auto& owner = m_controller->GetOwner();
	const Vec3& playerPos = m_controller->GetOwner()->GetPos();
	const Vec3& playerRot = owner->GetRotation();

	for(int i = 0; i < soldiersToSummon; ++i) {
		int soldierIndex = currentCount + i;

		int row = soldierIndex / kRowSize;
		int col = soldierIndex % kRowSize;

		Vec3 offset;
		offset.x = (static_cast<float>(col) - 2.0f) * kSpacing;  // 중앙 정렬
		offset.y = 0.0f;
		offset.z = (row + 1) * kSpacing;  // 뒤로 정렬

		const Vec3 targetPos = {
			playerPos.x + offset.x,
			playerPos.y + offset.y,
			playerPos.z + offset.z
		};

		auto iter = soldiers.begin();
		std::advance(iter, soldierIndex);
		auto& soldier = iter->second;

		if(soldier->GetComponent<Server::Contents::FSM>()->GetCurState()->GetStateType() == etou8(SOLDIER_STATE_TYPE::IDLE)) {
			soldier->GetComponent<Server::Contents::FSM>()->ChangeState(etou8(SOLDIER_STATE_TYPE::RUN));
			static_cast<Server::Contents::SoldierRunState*>(soldier->GetComponent<Server::Contents::FSM>()->GetCurState())->SetTargetPos(targetPos);
		}
	}
}
