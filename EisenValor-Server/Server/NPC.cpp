#include "pch.h"
#include "NPC.h"

#include "Player.h"
#include "GameRoom.h"

Server::Contents::NPC::NPC()
	:GameObject(GAME_OBJECT_TYPE::NPC)
{
	static uint32 idGen{ 10000 };
	SetID(idGen);
	idGen++;
}

void Server::Contents::NPC::Update(const float dt)
{
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
			float moveX = pos.x + dx * 5.f * dt;
			float moveY = pos.y + dy * 5.f * dt;
			float moveZ = pos.z + dz * 5.f * dt;

			SetPos(Vec3{ moveX, moveY, moveZ });
			SetRotation(targetRot);
			auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(GetID(), KinematicInfo{ GetPos(), GetRotation() });
			owner->GetGameWorld()->BroadcastInMatch(std::move(pb));
		}
	}
}