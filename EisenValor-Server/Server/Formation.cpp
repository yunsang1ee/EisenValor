#include "pch.h"
#include "Formation.h"

#include "TroopController.h"
#include "GameObject.h"
#include "NPC.h"
#include "SoldierState.h"

void Server::Contents::LineFormation::Init()
{
}

void Server::Contents::VShapeFormation::Init()
{

}

void Server::Contents::VShapeFormation::Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept
{
    if(soldiers.empty())
        return;

    const Vec3 center = m_centerPos;

    // 중심 방향 기준 forward, right 계산
    Vec3 forward = m_controller->GetOwner()->GetForward();   // 플레이어가 바라보는 방향 (Normalized)
    Vec3 right = Vec3(forward.z, 0.f, -forward.x); // yaw 기준 오른쪽 벡터
    forward.Normalize();
    right.Normalize();

	const float rowSpacing = 2.0f;   // 기본 앞뒤 간격	
	const float colSpacing = 2.0f;   // 좌우 간격
	const float vSpread = 1.0f;   // V자 펼침 강도 (좌우 벌어짐에 따른 추가 뒤로 밀림)

	int soldierIndex = 0;
	for(auto& [id, soldierData] : soldiers) {
		// 가상의 2D 그리드 인덱스
		int row = soldierIndex / 5;      // 한 줄에 5명 배치한다고 가정
		int col = soldierIndex % 5 - 2;  // -2 ~ +2 (중앙 기준)

		// V자 효과: col의 절댓값만큼 뒤로 더 밀림
		float backOffset = rowSpacing * row + vSpread * std::abs(col);

		Vec3 pos = center
			- forward * backOffset
			+ right * (colSpacing * col);

		soldierData.soldier->SetTargetPos(pos);

		++soldierIndex;
	}
}

void Server::Contents::LineFormation::Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept
{
	const int rowCount = 5;   // 세로 5줄
	const int colCount = 5;   // 가로 5줄
	const float spacing = 1.0f; // 병사 간격 (단위: m)

	// 첫 번째 병사가 설 위치 (좌측 상단 기준)
	float startX = m_centerPos.x - (colCount - 1) * spacing * 0.5f;
	float startZ = m_centerPos.z - (rowCount - 1) * spacing * 0.5f;

	int idx = 0;
	for(auto& [id, soldierData] : soldiers) {
		int row = idx / colCount;
		int col = idx % colCount;

		Vec3 pos;
		pos.x = startX + col * spacing;
		pos.y = m_centerPos.y;   // 높이는 동일하게
		pos.z = startZ + row * spacing;
		if((pos - soldierData.soldier->GetTargetPos()).LengthSquared() > 0.01f) {
			soldierData.soldier->SetTargetPos(pos);
		}
		idx++;
		if(idx >= rowCount * colCount)
			break; // 25명까지만 배치
	}
}

void Server::Contents::CircleFormation::Init()
{
}

void Server::Contents::CircleFormation::Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept
{
}
