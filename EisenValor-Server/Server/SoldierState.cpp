#include "pch.h"
#include "SoldierState.h"

#include "NPC.h"
#include "GameRoom.h"
#include "Player.h"
#include "FSM.h"

Server::Contents::SoldierIdleState::SoldierIdleState()
	:State(etou8(SOLDIER_STATE_TYPE::IDLE))
{
}

Server::Contents::SoldierIdleState::~SoldierIdleState()
{
}
 
void Server::Contents::SoldierIdleState::Enter()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter Soldier Idle", id) << std::endl;
}

void Server::Contents::SoldierIdleState::Exit()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter Soldier Idle", id) << std::endl;
}

uint8 Server::Contents::SoldierIdleState::Update(const float dt)
{
	return GetType();
}

Server::Contents::SoldierTraceState::SoldierTraceState()
	:State(etou8(SOLDIER_STATE_TYPE::TRACE))
{
}

Server::Contents::SoldierTraceState::~SoldierTraceState()
{
}

void Server::Contents::SoldierTraceState::Enter()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter Soldier Walk", id) << std::endl;
}

void Server::Contents::SoldierTraceState::Exit()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit Soldier Walk", id) << std::endl;
}

float DistanceSquared(const Vec3& a, const Vec3& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;

	return dx * dx + dy * dy + dz * dz;
}

uint8 Server::Contents::SoldierTraceState::Update(const float dt)
{
	// TODO:수정 필요

	//auto owner = GetFSM()->GetOwner();
	//
	//Vec3 curPos = owner->GetPos();
	//Vec3 target = m_targetPos;  // SetTargetPos()에서 지정한 목적지

	//Vec3 toTarget = target - curPos;
	//float distance = toTarget.Length();

	//// 병사 이동 속도 (초당 몇 m 이동할지)
	//constexpr float moveSpeed = 3.0f;

	//if(distance < 0.05f) {
	//	// 거의 도착하면 위치를 타겟에 고정하고 IDLE로 전환
	//	owner->SetPos(target);
	//	return etou8(SOLDIER_STATE_TYPE::IDLE);
	//}

	//// 방향 벡터 정규화
	//Vec3 dir = toTarget / distance;

	//// 이동할 거리 = 속도 * 시간
	//float moveDist = moveSpeed * dt;
	//if(moveDist > distance) moveDist = distance; // overshoot 방지

	//// 최종 위치
	//Vec3 newPos = curPos + dir * moveDist;
	//owner->SetPos(newPos);

	//// 회전도 목표 방향으로 보정 (y축 기준)
	//float newRotY = atan2(dir.x, dir.z);
	//Vec3 newRot = owner->GetRotation();
	//newRot.y = newRotY;
	//owner->SetRotation(newRot);

	//const uint32 id{ GetFSM()->GetOwner()->GetID() };
	//const Vec3 pos{ GetFSM()->GetOwner()->GetPos() };
	//const Vec3 rot{ GetFSM()->GetOwner()->GetRotation() };

	//auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
	//GetFSM()->GetOwner()->GetGameRoom()->Broadcast(std::move(pb));

	return GetType();
}

void Server::Contents::SoldierTraceState::Move(const float dt)
{

}

void Server::Contents::SoldierTraceState::MoveByForce(const float dt)
{
	auto owner = GetFSM()->GetOwner();
	if(!owner) return;

	Vec3 myPos = owner->GetPos();

	// 1. 목표 위치 방향
	Vec3 desiredDir{ m_targetPos - myPos };

	//if(desiredDir.Length() < 0.01f) {
	//    owner->GetComponent<Server::Contents::FSM>()->ChangeState(STATE_TYPE::IDLE);
	//    return;
	//}

	desiredDir.Normalize();

	// 2. Separation Force 계산
	Vec3 separationForce;
	constexpr float desiredSeparation{ 1.f };

	for(auto& [id, other] : owner->GetGameRoom()->GetNpcs()) {
		if(other == owner) continue;
		Vec3 diff = myPos - other->GetPos();
		float dist = diff.Length();

		if(dist < desiredSeparation && dist > 0.0001f) {
			diff.Normalize();
			separationForce += diff * ((desiredSeparation - dist) / desiredSeparation);
		}
	}

	// 3. 최종 방향 = 목표 + 회피
	Vec3 finalDir = desiredDir + separationForce;

	if(finalDir.Length() > 0.0001f)
		finalDir.Normalize();

	Vec3 smoothedDir = Lerp(m_prevDir, finalDir, 0.2f);
	m_prevDir = smoothedDir;


	constexpr float moveSpeed = 3.0f;
	Vec3 velocity = smoothedDir * moveSpeed;
	owner->SetVelocity(velocity);

	// 위치 갱신
	owner->SetPos(myPos + velocity * dt);

	//// 회전도 목표 방향으로 보정 (y축 기준)
	float newRotY = atan2(smoothedDir.x, smoothedDir.z);
	Vec3 newRot = owner->GetRotation();
	newRot.y = newRotY;
	owner->SetRotation(newRot);
}
