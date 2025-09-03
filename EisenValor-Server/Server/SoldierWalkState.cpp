#include "pch.h"
#include "SoldierWalkState.h"

#include "GameObject.h"
#include "FSM.h"
#include "GameRoom.h"

void Server::Contents::SoldierWalkState::Enter()
{
	// std::cout << "SoldierWalkEnter" << std::endl;
}

void Server::Contents::SoldierWalkState::Exit()
{
	// std::cout << "SoldierWalkExit" << std::endl;
}

float DistanceSquared(const Vec3& a, const Vec3& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;

	return dx * dx + dy * dy + dz * dz;
}

void Server::Contents::SoldierWalkState::Update(const float dt)
{
    auto owner = GetFSM()->GetOwner();
    if(!owner) return;

    Vec3 curPos = owner->GetPos();
    Vec3 target = m_targetPos;  // SetTargetPos()에서 지정한 목적지

    Vec3 toTarget = target - curPos;
    float distance = toTarget.Length();

    // 병사 이동 속도 (초당 몇 m 이동할지)
    constexpr float moveSpeed = 3.0f;

    if(distance < 0.05f) {
        // 거의 도착하면 위치를 타겟에 고정하고 IDLE로 전환
        owner->SetPos(target);
        GetFSM()->ChangeState(STATE_TYPE::IDLE);
        return;
    }

    // 방향 벡터 정규화
    Vec3 dir = toTarget / distance;

    // 이동할 거리 = 속도 * 시간
    float moveDist = moveSpeed * dt;
    if(moveDist > distance) moveDist = distance; // overshoot 방지

    // 최종 위치
    Vec3 newPos = curPos + dir * moveDist;
    owner->SetPos(newPos);

    // 회전도 목표 방향으로 보정 (y축 기준)
    float newRotY = atan2(dir.x, dir.z);
    Vec3 newRot = owner->GetRotation();
    newRot.y = newRotY;
    owner->SetRotation(newRot);

	const uint32 id{ GetFSM()->GetOwner()->GetID() };
	const Vec3 pos{ GetFSM()->GetOwner()->GetPos() };
	const Vec3 rot{ GetFSM()->GetOwner()->GetRotation() };

	auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
	GetFSM()->GetOwner()->GetGameRoom()->Broadcast(std::move(pb));
}

