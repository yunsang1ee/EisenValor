#include "pch.h"
#include "SoldierStates.h"

#include "NPC.h"
#include "GameRoom.h"
#include "Player.h"
#include "FSM.h"
#include "Team.h"

Server::Contents::SoldierIdleState::SoldierIdleState()
	:IdleState{ etou8(SOLDIER_STATE_TYPE::IDLE) }
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

void Server::Contents::SoldierIdleState::Update(const float dt)
{
	//const auto fsm = GetFSM();
	//const auto& owner = fsm->GetOwner();
	//const auto& onwerPos = owner->GetPos();
	//const Vec3& targetPos = std::static_pointer_cast<NPC>(owner)->GetTargetPos();

	//const Vec3 diff = targetPos - m_prevTargetPos;

	//if(diff.LengthSquared() >= 0.001f) {
	//	// 주변에 적이 있으면 Attack, 아니면 RUN
	//	fsm->ChangeState(etou8(SOLDIER_STATE_TYPE::RUN));
	//}
	//else {
	//	
	//}


	// 1. 모든 상대방을 탐색한다.
	// 2. 
	const auto fsm = GetFSM();
	const auto owner = fsm->GetOwner();
	const auto room = owner->GetGameRoom();
	const auto otherTeam = room->GetOtherTeam(owner->GetTeamType());

	const auto& ownerPos = owner->GetPos();

	auto& otherTeamObjectGroup = room->GetTeam(otherTeam).GetAllObjectGroups();

	for(int i = 0; i < otherTeamObjectGroup.size(); ++i) {
		if(i == etou8(GAME_OBJECT_TYPE::PROJECTILE) || i == etou8(GAME_OBJECT_TYPE::VALLISTAR)) continue;
		
		for(auto& [id, object] : otherTeamObjectGroup[i]) {

			const auto target = std::static_pointer_cast<Creature>(object);
			const auto& targetPos = object->GetPos();

			const float distSq = (targetPos - ownerPos).LengthSquared();
			const float detectionEnemyRangeSq = detectionEnemyRange * detectionEnemyRange;

			if(distSq <= detectionEnemyRangeSq) {
				std::static_pointer_cast<Creature>(owner)->SetTarget(target);
				fsm->ChangeState(etou8(SOLDIER_STATE_TYPE::RUN));
			}
		}
	}
}

Server::Contents::SoldierRunState::SoldierRunState()
	:RunState{ etou8(SOLDIER_STATE_TYPE::RUN) }
{
}

Server::Contents::SoldierRunState::~SoldierRunState()
{
}

void Server::Contents::SoldierRunState::Enter()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierRunState", id) << std::endl;
}

void Server::Contents::SoldierRunState::Exit()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierRunState", id) << std::endl;
}

void Server::Contents::SoldierRunState::Update(const float dt)
{
	const auto fsm = GetFSM();
	const auto owner = fsm->GetOwner();
	const Vec3& ownerPos = owner->GetPos();

	const auto target = std::static_pointer_cast<Creature>(owner)->GetTarget();
	if(target) {
		// TODO: 타겟을 향해 쫓아감
		// TODO: 공격 사거리 안에 들어오면 AttackState로 간다.
		const auto& targetPos = target->GetPos();

		const Vec3 toTarget = (targetPos - ownerPos);
		
		const float distToTarget = toTarget.Length();

		if(distToTarget < 0.3f) {
			fsm->ChangeState(etou8(SOLDIER_STATE_TYPE::ATTACK));
			return;
		}
		else {
			float moveDist = 1.f * dt;

			const Vec3 dir = toTarget / distToTarget;

			if(moveDist > distToTarget) moveDist = distToTarget;
			Vec3 newPos{ ownerPos + dir * moveDist };
			owner->SetPos(newPos);
		}
	}
	else {
		
	}

	//auto owner = GetFSM()->GetOwner();
	//
	//Vec3 curPos = owner->GetPos();
	//Vec3 target = std::static_pointer_cast<NPC>(owner)->GetTargetPos();  // SetTargetPos()에서 지정한 목적지

	//Vec3 toTarget = target - curPos;
	//float distance = toTarget.Length();

	//// 병사 이동 속도 (초당 몇 m 이동할지)
	//constexpr float moveSpeed = 3.0f;

	//if(distance < 0.01f) {
	//	// 거의 도착하면 위치를 타겟에 고정하고 IDLE로 전환
	//	owner->SetPos(target);
	//	GetFSM()->ChangeState(etou8(SOLDIER_STATE_TYPE::IDLE));
	//	const uint32 id{ GetFSM()->GetOwner()->GetID() };
	//	const Vec3 pos{ GetFSM()->GetOwner()->GetPos() };
	//	const Vec3 rot{ GetFSM()->GetOwner()->GetRotation() };

	//	auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
	//	GetFSM()->GetOwner()->GetGameRoom()->BroadcastToAll(std::move(pb));
	//	return;
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

	const uint32 id{ GetFSM()->GetOwner()->GetID() };
	const Vec3 pos{ GetFSM()->GetOwner()->GetPos() };
	const Vec3 rot{ GetFSM()->GetOwner()->GetRotation() };

	auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
	GetFSM()->GetOwner()->GetGameRoom()->BroadcastToAll(std::move(pb));
}

//Server::Contents::SoldierTraceState::SoldierTraceState()
//	:State(etou8(SOLDIER_STATE_TYPE::TRACE))
//{
//}
//
//Server::Contents::SoldierTraceState::~SoldierTraceState()
//{
//}
//
//void Server::Contents::SoldierTraceState::Enter()
//{
//	const auto& owner = GetFSM()->GetOwner();
//	const uint32 id{ owner->GetID() };
//	std::cout << std::format("ID = {}, Enter Soldier Walk", id) << std::endl;
//}
//
//void Server::Contents::SoldierTraceState::Exit()
//{
//	const auto& owner = GetFSM()->GetOwner();
//	const uint32 id{ owner->GetID() };
//	std::cout << std::format("ID = {}, Exit Soldier Walk", id) << std::endl;
//}
//
//float DistanceSquared(const Vec3& a, const Vec3& b)
//{
//	float dx = a.x - b.x;
//	float dy = a.y - b.y;
//	float dz = a.z - b.z;
//
//	return dx * dx + dy * dy + dz * dz;
//}
//
//uint8 Server::Contents::SoldierTraceState::Update(const float dt)
//{
//	// TODO:수정 필요
//
//	//auto owner = GetFSM()->GetOwner();
//	//
//	//Vec3 curPos = owner->GetPos();
//	//Vec3 target = m_targetPos;  // SetTargetPos()에서 지정한 목적지
//
//	//Vec3 toTarget = target - curPos;
//	//float distance = toTarget.Length();
//
//	//// 병사 이동 속도 (초당 몇 m 이동할지)
//	//constexpr float moveSpeed = 3.0f;
//
//	//if(distance < 0.05f) {
//	//	// 거의 도착하면 위치를 타겟에 고정하고 IDLE로 전환
//	//	owner->SetPos(target);
//	//	return etou8(SOLDIER_STATE_TYPE::IDLE);
//	//}
//
//	//// 방향 벡터 정규화
//	//Vec3 dir = toTarget / distance;
//
//	//// 이동할 거리 = 속도 * 시간
//	//float moveDist = moveSpeed * dt;
//	//if(moveDist > distance) moveDist = distance; // overshoot 방지
//
//	//// 최종 위치
//	//Vec3 newPos = curPos + dir * moveDist;
//	//owner->SetPos(newPos);
//
//	//// 회전도 목표 방향으로 보정 (y축 기준)
//	//float newRotY = atan2(dir.x, dir.z);
//	//Vec3 newRot = owner->GetRotation();
//	//newRot.y = newRotY;
//	//owner->SetRotation(newRot);
//
//	//const uint32 id{ GetFSM()->GetOwner()->GetID() };
//	//const Vec3 pos{ GetFSM()->GetOwner()->GetPos() };
//	//const Vec3 rot{ GetFSM()->GetOwner()->GetRotation() };
//
//	//auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
//	//GetFSM()->GetOwner()->GetGameRoom()->Broadcast(std::move(pb));
//
//	return GetStateType();
//}
//
//void Server::Contents::SoldierTraceState::Move(const float dt)
//{
//
//}
//
//void Server::Contents::SoldierTraceState::MoveByForce(const float dt)
//{
//	auto owner = GetFSM()->GetOwner();
//	if(!owner) return;
//
//	Vec3 myPos = owner->GetPos();
//
//	// 1. 목표 위치 방향
//	Vec3 desiredDir{ m_targetPos - myPos };
//
//	//if(desiredDir.Length() < 0.01f) {
//	//    owner->GetComponent<Server::Contents::FSM>()->ChangeState(STATE_TYPE::IDLE);
//	//    return;
//	//}
//
//	desiredDir.Normalize();
//
//	// 2. Separation Force 계산
//	Vec3 separationForce;
//	constexpr float desiredSeparation{ 1.f };
//
//	for(auto& [id, other] : owner->GetGameRoom()->GetNpcs()) {
//		if(other == owner) continue;
//		Vec3 diff = myPos - other->GetPos();
//		float dist = diff.Length();
//
//		if(dist < desiredSeparation && dist > 0.0001f) {
//			diff.Normalize();
//			separationForce += diff * ((desiredSeparation - dist) / desiredSeparation);
//		}
//	}
//
//	// 3. 최종 방향 = 목표 + 회피
//	Vec3 finalDir = desiredDir + separationForce;
//
//	if(finalDir.Length() > 0.0001f)
//		finalDir.Normalize();
//
//	Vec3 smoothedDir = Lerp(m_prevDir, finalDir, 0.2f);
//	m_prevDir = smoothedDir;
//
//
//	constexpr float moveSpeed = 3.0f;
//	Vec3 velocity = smoothedDir * moveSpeed;
//	owner->SetVelocity(velocity);
//
//	// 위치 갱신
//	owner->SetPos(myPos + velocity * dt);
//
//	//// 회전도 목표 방향으로 보정 (y축 기준)
//	float newRotY = atan2(smoothedDir.x, smoothedDir.z);
//	Vec3 newRot = owner->GetRotation();
//	newRot.y = newRotY;
//	owner->SetRotation(newRot);
//}

Server::Contents::SoldierAttackState::SoldierAttackState()
	:AttackState{etou8(SOLDIER_STATE_TYPE::ATTACK)}
{
}

Server::Contents::SoldierAttackState::~SoldierAttackState()
{
}

void Server::Contents::SoldierAttackState::Enter()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierAttackState", id) << std::endl;
}

void Server::Contents::SoldierAttackState::Exit()
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierAttackState", id) << std::endl;
}

void Server::Contents::SoldierAttackState::Update(const float dt)
{
	// TODO: 
}
