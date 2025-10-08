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
	std::cout << std::format("ID = {}, Exit Soldier Idle", id) << std::endl;
}

void Server::Contents::SoldierIdleState::Update(const float dt)
{
	const auto fsm = GetFSM();
	const auto owner = fsm->GetOwner();
	const auto room = owner->GetGameRoom();
	const auto otherTeam = room->GetOtherTeamType(owner->GetTeamType());
	const auto& ownerPos = owner->GetPos();
	auto& otherTeamObjectGroup = room->GetTeam(otherTeam).GetAllObjectGroups();

	for(int i = 0; i < otherTeamObjectGroup.size(); ++i) {
		if(i == FB_ENUMS::GAME_OBJECT_TYPE_PROJECTILE || i == FB_ENUMS::GAME_OBJECT_TYPE_VALLISTAR) continue;
		
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
	const auto owner = std::static_pointer_cast<NPC>(fsm->GetOwner());
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
		// 타겟이 없으면 현재 보고있는 방향으로 달려감
		// 달려가다가 주변에 적이 있으면 맨 처음 본 적을 Target으로 설정
		const float moveDist = 1.f * dt;
		const Vec3 dir = owner->GetForward();
		Vec3 newPos{ ownerPos + dir * moveDist };
		owner->SetPos(newPos);

		auto otherTeamType = owner->GetGameRoom()->GetOtherTeamType(owner->GetTeamType());
		auto& otherTeam = owner->GetGameRoom()->GetTeam(otherTeamType);
		
		for(auto& [id, obj] : otherTeam.GetNpcs()) {
			const auto npc = std::static_pointer_cast<NPC>(obj);
			const auto npcPos = npc->GetPos();

			if(obj->GetNpcType() == FB_ENUMS::NPC_TYPE_SOLDIER) {
				const Vec3 toNpc= (npcPos - ownerPos);
				const float distToNpc = toNpc.Length();
				if(distToNpc < 0.5f) {
					owner->SetTarget(npc);
				}
			}
		}
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

	//const uint32 id{ GetFSM()->GetOwner()->GetID() };
	//const Vec3 pos{ GetFSM()->GetOwner()->GetPos() };
	//const Vec3 rot{ GetFSM()->GetOwner()->GetRotation() };

	//auto pb = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
	//GetFSM()->GetOwner()->GetGameRoom()->BroadcastToAll(std::move(pb));
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
	m_accDt += dt;
	const auto& owner = std::static_pointer_cast<NPC>(GetFSM()->GetOwner());
	const uint32 id = owner->GetID();

	if(m_accDt >= static_cast<float>(ATTACK_TIME.count())) {
		std::cout << std::format("ID = {}, Attack!", id) << std::endl;
		auto target = owner->GetTarget();
		if(target) {
			int32 targetHp = owner->GetTarget()->GetHP();
			targetHp -= owner->GetAtk();
			owner->GetTarget()->SetHp(targetHp);
			int32 ownerStamina = owner->GetStamina();
			ownerStamina -= 5;
			owner->SetStamina(ownerStamina);

			auto pb = ServerPackets::Make_SC_HIT_PACKET(std::static_pointer_cast<Server::Contents::Creature>(target)->GetID(), std::static_pointer_cast<Server::Contents::Creature>(target)->GetHP());
			owner->GetGameRoom()->ExecuteAsyncronously(&GameRoom::BroadcastToAll, std::move(pb));
			if(targetHp <= 0) {
				target->SetAlive(false);
				target->GetGameRoom()->AddEvent([t = std::move(target)]()
					{
						// t->SetAlive(false);
						t->GetGameRoom()->GetTeam(t->GetTeamType()).RemoveObject(t);
					});
			}
			m_accDt = 0.f;
		}
		else {
			GetFSM()->ChangeState(etou8(SOLDIER_STATE_TYPE::IDLE));
		}
	}
}

Server::Contents::SoldierDefenseState::SoldierDefenseState()
	:DefenseState{ etou8(SOLDIER_STATE_TYPE::DEFENSE) }
{

}

Server::Contents::SoldierDefenseState::~SoldierDefenseState()
{

}

void Server::Contents::SoldierDefenseState::Enter()
{

}

void Server::Contents::SoldierDefenseState::Exit()
{

}

void Server::Contents::SoldierDefenseState::Update(const float dt)
{
	// TODO: Soldier Attack State Update
}
