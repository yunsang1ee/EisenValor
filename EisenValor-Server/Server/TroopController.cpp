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
	if(!m_curFormation){
		return;
	}

	// 1. 플레이어 위치 변화 추적
	const Vec3& curOwnerPos = GetOwner()->GetPos();
	static Vec3 prevOwnerPos = curOwnerPos;

	Vec3 delta = curOwnerPos - prevOwnerPos;
	prevOwnerPos = curOwnerPos;

	// 길이 제곱 직접 계산 (delta.LengthSq() 대신)
	float lenSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

	if(lenSq > 0.0001f) {
		// 플레이어 이동량만큼 대열 중심도 이동
		//m_targetPos.x += delta.x;
		//m_targetPos.y += delta.y;
		//m_targetPos.z += delta.z;
	}
	else {
		return;
	}
	
	// 2. 현재 formation에 맞춰 병사 위치 갱신
	m_curFormation->Arrange(m_soldiers);
}

void Server::Contents::TroopController::SetFormation(const TROOP_FORMATION_TYPE type) noexcept
{
	m_curFormation = m_formations.find(type)->second.get();
	m_curFormation->Arrange(m_soldiers);
	std::cout << "SET FORMATION" << std::endl;
}
 
void Server::Contents::TroopController::SetTargetPos(const Vec3& targetPos)
{
	/*m_targetPos = targetPos;
	if(m_curFormation)
		m_curFormation->Arrange(m_soldiers);*/
}

void Server::Contents::LineFormation::Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept
{
	// 현재 플레이어 수 계산
	// 각 플레이어의 대열 위치 계산 -> 타겟 지점으로 설정
	//const int maxSoldiers = soldiers.size();
	//for(auto& [id, soldierData] : soldiers) {
	//	
	//}

	//const int kMaxSoldierCount = soldiers.size();
	//constexpr int kRowSize = 5;
	//constexpr float kSpacing = 3.f;
	//int currentCount{};

	//int soldiersToSummon = kMaxSoldierCount;
	//
	//const auto& owner = m_controller->GetOwner();
	//const Vec3& playerPos = m_controller->GetOwner()->GetPos();
	//const Vec3& playerRot = owner->GetRotation();

	//for(int i = 0; i < soldiersToSummon; ++i) {
	//	int soldierIndex = currentCount + i;

	//	int row = soldierIndex / kRowSize;
	//	int col = soldierIndex % kRowSize;

	//	Vec3 offset;
	//	offset.x = (static_cast<float>(col) - 2.0f) * kSpacing;  // 중앙 정렬
	//	offset.y = 0.0f;
	//	offset.z = (row + 1) * kSpacing;  // 뒤로 정렬

	//	const Vec3 targetPos = {
	//		playerPos.x + offset.x,
	//		playerPos.y + offset.y,
	//		playerPos.z + offset.z
	//	};

	//	auto iter = soldiers.begin();
	//	std::advance(iter, soldierIndex);
	//	auto& soldier = iter->second;

	//	//if(soldier->GetComponent<Server::Contents::FSM>()->GetCurState()->GetStateType() == etou8(SOLDIER_STATE_TYPE::IDLE)) {
	//	//	soldier->GetComponent<Server::Contents::FSM>()->ChangeState(etou8(SOLDIER_STATE_TYPE::RUN));
	//	//	static_cast<Server::Contents::SoldierRunState*>(soldier->GetComponent<Server::Contents::FSM>()->GetCurState())->SetTargetPos(targetPos);
	//	//}
	//}

	if(soldiers.empty())
		return;

	constexpr int kRowSize = 5;       // 한 줄 최대 병사 수
	constexpr float kSpacing = 3.f;   // 병사 간격

	int idx = 0;
	for(auto& [id, soldierData] : soldiers) {
		int row = idx / kRowSize;
		int col = idx % kRowSize;

		// 대열 내 상대 위치 (offset)
		float offsetX = (static_cast<float>(col) - (kRowSize - 1) / 2.0f) * kSpacing;
		float offsetZ = row * kSpacing;
		soldierData.offset = Vec3{ offsetX, 0.f, offsetZ };

		// 최종 월드 좌표 = 대열 중심(m_targetPos) + offset
		//const Vec3 targetPos = {
		//	m_controller->m_targetPos.x + offsetX,
		//	m_controller->m_targetPos.y,
		//	m_controller->m_targetPos.z + offsetZ
		//};

		auto& soldier = soldierData.soldier;
		if(!soldier) continue;

		auto fsm = soldier->GetComponent<Server::Contents::FSM>();
		if(!fsm) continue;

		auto curState = fsm->GetCurState();
		if(!curState || curState->GetStateType() != etou8(SOLDIER_STATE_TYPE::RUN))
			fsm->ChangeState(etou8(SOLDIER_STATE_TYPE::RUN));

		// 병사의 목표 위치 업데이트
		// static_cast<Server::Contents::SoldierRunState*>(fsm->GetCurState())->SetTargetPos(targetPos);

		++idx;
	}
}

