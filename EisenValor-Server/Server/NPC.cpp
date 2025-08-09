#include "pch.h"
#include "NPC.h"

#include "Player.h"
#include "GameWorld.h"

Server::Contents::NPC::NPC()
	:GameObject(GAME_OBJECT_TYPE::NPC)
{
	static uint32 idGen{ 10000 };
	SetID(idGen);
	idGen++;
}

void Server::Contents::NPC::Update()
{
	auto now = std::chrono::high_resolution_clock::now();
	float DT = 0.f;
	if(m_firstUpdate) m_firstUpdate = false;
	else {
		DT = std::chrono::duration<float>(now - m_lastUpdate).count();
	}
	m_lastUpdate = now;

	//if(auto owner = m_owner.lock()) {
	//    Vec3 ownerPos = owner->GetPos();
	//    Vec3 ownerRotEuler = owner->GetRotation();

	//    // 2) 회전 매트릭스로 오프셋 회전
	//    float yaw = owner->GetRotation().y;  // 라디안 단위라고 가정
	//    float c = std::cos(yaw), s = std::sin(yaw);

	//    // 로컬 오프셋.x → X방향, .z → Z방향
	//    float worldX = ownerPos.x + m_formationOffset.x * c - m_formationOffset.z * s;
	//    float worldZ = ownerPos.z + m_formationOffset.x * s + m_formationOffset.z * c;
	//    float worldY = ownerPos.y + m_formationOffset.y;

	//    Vec3 desiredPos{ worldX, worldY, worldZ };

	//    Vec3 curPos = GetPos();
	//    Vec3 diff = desiredPos - curPos;
	//    float dist = diff.Length();

	//    if(dist > 0.01f && DT > 0.f) {
	//        // 3) 이동량 계산
	//        Vec3 dir = diff / dist;
	//        constexpr float speed = 5.f;
	//        float moveDist = speed * DT;
	//        if(moveDist > dist) moveDist = dist;

	//        Vec3 newPos = curPos + dir * moveDist;
	//        SetPos(newPos);

	//        // 4) 회전 동기화 (플레이어 바라보기 or 이동 방향)
	//        SetRotation(ownerRotEuler);
	//        const auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(GetID(), KinematicInfo{ GetPos(), GetRotation() });
	//        owner->GetGameWorld()->BroadcastInMatch(pb);
	//    }
	//}

	if(auto owner = m_owner.lock()) {
		const Vec3 targetPos = owner->GetPos();
		const Vec3 targetRot = owner->GetRotation();
		const Vec3 pos = GetPos();

		//// 거리 계산
		float dx = targetPos.x - pos.x;
		float dy = targetPos.y - pos.y;
		float dz = targetPos.z - pos.z;
		float distance = sqrt(dx * dx + dy * dy + dz * dz);

		//// 일정 거리 이상이면 따라가기
		if(distance > 1.f) {
			// 방향 벡터 정규화
			dx /= distance;
			dy /= distance;
			dz /= distance;

			// 새 위치 계산 (살짝 부드럽게 이동)
			float moveX = pos.x + dx * 5.f * DT;
			float moveY = pos.y + dy * 5.f * DT;
			float moveZ = pos.z + dz * 5.f * DT;

			SetPos(Vec3{ moveX, moveY, moveZ });
			SetRotation(targetRot);
			auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(GetID(), KinematicInfo{ GetPos(), GetRotation() });
			owner->GetGameWorld()->BroadcastInMatch(std::move(pb));
		}
	}

	ExecuteAfterTime(std::chrono::milliseconds(20), &NPC::Update);
}