#include "pch.h"
#include "SoldierStates.h"

#include "NPC.h"
#include "GameRoom.h"
#include "Player.h"
#include "FSM.h"
#include "Team.h"

Server::Contents::SoldierIdleState::SoldierIdleState()
	:IdleState{ FB_ENUMS::SOLDIER_STATE_TYPE_IDLE }
{
}

Server::Contents::SoldierIdleState::~SoldierIdleState()
{
}

void Server::Contents::SoldierIdleState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter Soldier Idle", id) << std::endl;
}

void Server::Contents::SoldierIdleState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit Soldier Idle", id) << std::endl;
}

void Server::Contents::SoldierIdleState::Update(const float dt)
{
	//const auto fsm = GetFSM();
	//const auto owner = fsm->GetOwner();
	//const auto room = owner->GetGameRoom();
	//const auto otherTeam = room->GetOtherTeamType(owner->GetTeamType());
	//const auto& ownerPos = owner->GetPos();
	//auto& otherTeamObjectGroup = room->GetTeam(otherTeam).GetAllObjectGroups();
	//for(int i = 0; i < otherTeamObjectGroup.size(); ++i) {
	//	if(i == FB_ENUMS::GAME_OBJECT_TYPE_PROJECTILE || i == FB_ENUMS::GAME_OBJECT_TYPE_VALLISTAR) continue;

	//	for(auto& [id, object] : otherTeamObjectGroup[i]) {

	//		const auto target = std::static_pointer_cast<Creature>(object);
	//		const auto& targetPos = object->GetPos();

	//		const float distSq = (targetPos - ownerPos).LengthSquared();
	//		const float detectionEnemyRangeSq = enemyDetectionRange * enemyDetectionRange;

	//		if(distSq <= detectionEnemyRangeSq) {
	//			std::cout << "IDLE -> RUN" << std::endl;
	//			std::static_pointer_cast<Creature>(owner)->SetTarget(target);
	//			fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt);
	//			return;
	//		}
	//	}
	//}

	// 1. 주변에 적이 있나?
	// -Y: 타겟을 설정하고 Chase로 전환
	// -N: Move로 전환

	const auto fsm = GetFSM();
	const auto owner = fsm->GetOwner();
	const auto room = owner->GetGameRoom();
	const auto otherTeam = room->GetOtherTeamType(owner->GetTeamType());
	const auto& ownerPos = owner->GetPos();
	auto& otherTeamObjectGroup = room->GetTeam(otherTeam).GetAllObjectGroups();

	// 주변에 적이 있는지 검사
	bool findEnemy{ false };
	for(int i = 0; i < otherTeamObjectGroup.size(); ++i) {
		if(i == FB_ENUMS::GAME_OBJECT_TYPE_PROJECTILE ||
			i == FB_ENUMS::GAME_OBJECT_TYPE_VALLISTAR || 
			i==FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER) continue;

		for(auto& [id, object] : otherTeamObjectGroup[i]) {
			const auto target = std::static_pointer_cast<Creature>(object);
			const auto& targetPos = object->GetPos();
			Vec3 targetDir = targetPos - ownerPos;

			const float distSq = (targetPos - ownerPos).LengthSquared();
			const float detectionEnemyRangeSq = enemyDetectionRange * enemyDetectionRange;

			// 탐지 범위 안에 적이 있으면
			// -> 타겟 설정, Chase로 상태 변화
			if(distSq <= detectionEnemyRangeSq) {
				std::cout << "IDLE -> CHASE" << std::endl;
				std::static_pointer_cast<Creature>(owner)->SetTarget(target);
				targetDir.Normalize();

				Vec3 rot{};
				rot.y = std::atan2(targetDir.x, targetDir.y) * DEG2RAD;
				owner->SetRotation(rot);
				fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE, dt);
				findEnemy = true;
				return;
			}
		}
	}
}

Server::Contents::SoldierMoveState::SoldierMoveState()
	:MoveState{ FB_ENUMS::SOLDIER_STATE_TYPE_MOVE }
{
}

Server::Contents::SoldierMoveState::~SoldierMoveState()
{
}

void Server::Contents::SoldierMoveState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierRunState", id) << std::endl;
}

void Server::Contents::SoldierMoveState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierRunState", id) << std::endl;
}

void Server::Contents::SoldierMoveState::Update(const float dt)
{
	// 1. 전략지의 일정 범위 내인가? 
	//	-> 전략지의 일정 범위를 어떻게 판단하는게 좋을까
	//	--> 1. 전략지 오브젝트를 따로 두어 검사한다

	// -Y: IDLE 상태로 전환
	// -N: 전략지로 이동한다
	//	-> 목표위치는 전략지이고, NavGraph상에서 Astar로 길찾기

	// 근데 현재는 전략지를 둘 수가 없다.
	
	//const auto fsm = GetFSM();
	//const auto owner = std::static_pointer_cast<NPC>(fsm->GetOwner());
	//const Vec3& ownerPos = owner->GetPos();

	//const auto target = std::static_pointer_cast<Creature>(owner)->GetTarget();
	//if(target) {
	//	const auto& targetPos = target->GetPos();

	//	const Vec3 toTarget = (targetPos - ownerPos);

	//	const float distToTarget = toTarget.Length();

	//	if(distToTarget < combatRange) {
	//		static constexpr float ATTACK_PROB{ 0.6f };
	//		static std::default_random_engine dre{ std::random_device{}() };
	//		static std::uniform_real_distribution<float> dist{ 0.f, 1.f };
	//		if(dist(dre) <= ATTACK_PROB) {
	//			std::cout << "RUN -> ATTACK" << std::endl;
	//			fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt);
	//			return;
	//		}
	//		else {
	//			std::cout << "RUN -> DEFENSE" << std::endl;
	//			fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE, dt);
	//			return;
	//		}
	//	}
	//	else {
	//		float moveDist = 3.f * dt;

	//		const Vec3 dir = toTarget / distToTarget;

	//		if(moveDist > distToTarget) moveDist = distToTarget;

	//		Vec3 newPos{ ownerPos + dir * moveDist };

	//		owner->SetPos(newPos);
	//	}
	//}
	//else {
	//	// std::cout << "RUN -> IDLE" << std::endl;
	//	// fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE::SOLDIER_STATE_TYPE_IDLE, dt);
	//	// return;
	//	// 타겟이 없으면 현재 보고있는 방향으로 달려감
	//	// 달려가다가 주변에 적이 있으면 맨 처음 본 적을 Target으로 설정


	//	// FIX: 주변에 적이 있는데도 불구하고 타겟이 없다고 판정되어 직진으로 가는 경우가 존재함.

	//	const float moveDist = 1.f * dt;
	//	const Vec3 dir = owner->GetForwardDir();
	//	Vec3 newPos{ ownerPos + dir * moveDist };
	//	owner->SetPos(newPos);

	//	auto otherTeamType = owner->GetGameRoom()->GetOtherTeamType(owner->GetTeamType());
	//	auto& otherTeam = owner->GetGameRoom()->GetTeam(otherTeamType);

	//	for(auto& [id, obj] : otherTeam.GetNpcs()) {
	//		const auto npc = std::static_pointer_cast<NPC>(obj);
	//		const auto npcPos = npc->GetPos();

	//		if(obj->GetNpcType() == FB_ENUMS::NPC_TYPE_SOLDIER) {
	//			const Vec3 toNpc = (npcPos - ownerPos);
	//			const float distToNpc = toNpc.Length();
	//			if(distToNpc < 0.5f) {
	//				owner->SetTarget(npc);
	//			}
	//		}
	//	}
	//}
}

Server::Contents::SoldierChaseState::SoldierChaseState()
	:ChaseState{ FB_ENUMS::SOLDIER_STATE_TYPE_CHASE }
{
}

Server::Contents::SoldierChaseState::~SoldierChaseState()
{
}

void Server::Contents::SoldierChaseState::Enter(const float dt)
{

}

void Server::Contents::SoldierChaseState::Exit(const float dt)
{

}

void Server::Contents::SoldierChaseState::Update(const float dt)
{
	const auto& owner = std::static_pointer_cast<Server::Contents::NPC>(GetFSM()->GetOwner());
	const auto& room = owner->GetGameRoom();

	const auto fsm = owner->GetComponent<FSM>();
	const auto& ownerPos = owner->GetPos();

	if(owner) {
		const auto& target = owner->GetTarget();

		// 타겟이 존재하면
		if(target) {
			const auto& targetPos = target->GetPos();
			const float distToTargetSq = (targetPos - ownerPos).LengthSquared();
			
			// 타겟이 전투범위 안에 들어왔으면
			// -> 공격 상태로 넘어간다
			if(distToTargetSq <= std::pow(combatRange, 2)) {
				fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt);
				return;
			}
			// 타겟이 아직 전투범위 안에 들어오지 않았으면
			// -> 타겟을 향해 이동한다
			else {
				float moveDist = chaseSpeed * dt;
				const Vec3 toTarget = targetPos - ownerPos;
				const float distToTarget = toTarget.Length();
				const Vec3 dir = toTarget / distToTarget;

				if(moveDist > distToTarget)
					moveDist = distToTarget;
				Vec3 newPos{ ownerPos + dir * moveDist };
				
				owner->SetPos(newPos);
			}
		}
		// 타겟이 존재하지 않으면
		// -> IDLE 상태로 돌아간다
		else {
			fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE, dt);
			return;
		}
	}
}


Server::Contents::SoldierAttackState::SoldierAttackState()
	:AttackState{ FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK }
{
}

Server::Contents::SoldierAttackState::~SoldierAttackState()
{
}

void Server::Contents::SoldierAttackState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierAttackState", id) << std::endl;
}

void Server::Contents::SoldierAttackState::Exit(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Exit SoldierAttackState", id) << std::endl;
	m_accDt = 0.f;
}

void Server::Contents::SoldierAttackState::Update(const float dt)
{
	const auto& owner = std::static_pointer_cast<NPC>(GetFSM()->GetOwner());
	const uint32 id = owner->GetID();
	m_accDt += dt;

	// 공격시간이 되었으면
	if(m_accDt >= static_cast<float>(attackCycleTime.count())) {
		auto target = owner->GetTarget();

		// 타겟이 존재한다면
		if(target) {
			switch(const auto objType = target->GetObjType()) {
				case FB_ENUMS::GAME_OBJECT_TYPE_NPC:
				{
					switch(const auto npcType = std::static_pointer_cast<NPC>(target)->GetNpcType()) {
						case FB_ENUMS::NPC_TYPE_GENERAL:
						{
							break;
						}
						case FB_ENUMS::NPC_TYPE_SOLDIER:
						{
							const auto targetState = target->GetComponent<FSM>()->GetCurState()->GetStateType();
							if(targetState == FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE) {
								std::cout << "상대가 방어 상태라 공격 못했음!" << std::endl;
								m_accDt = 0.f;
								return;
							}
							else {
								int32 targetHp = owner->GetTarget()->GetHP();
								targetHp -= owner->GetAtk();
								owner->GetTarget()->SetHp(targetHp);
								std::cout << std::format("ID = {}, Attack!", id) << std::endl;
								if(targetHp <= 0) {
									target->SetAlive(false);
									target->GetGameRoom()->AddEvent([t = target]()
										{
											// t->SetAlive(false);
											t->GetGameRoom()->GetTeam(t->GetTeamType()).RemoveObject(t);
										});
								}
								const auto fsm = GetFSM();

								static constexpr float ATTACK_PROB{ 0.6f };
								static std::default_random_engine dre{ std::random_device{}() };
								static std::uniform_real_distribution<float> dist{ 0.f, 1.f };
								if(dist(dre) <= ATTACK_PROB) {
									m_accDt = 0.f;
								}
								else {
									std::cout << "ATTACK -> DEFENSE" << std::endl;
									fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE, dt);
								}
							}
							break;
						}
						default:
							break;
					}
					break;
				}
				case FB_ENUMS::GAME_OBJECT_TYPE_PLAYER:
				{
					int32 targetHp = owner->GetTarget()->GetHP();
					targetHp -= owner->GetAtk();
					owner->GetTarget()->SetHp(targetHp);
					if(targetHp <= 0) {
						target->SetAlive(false);
						target->GetGameRoom()->AddEvent([t = target]()
							{
								// t->SetAlive(false);
								t->GetGameRoom()->GetTeam(t->GetTeamType()).RemoveObject(t);
							});
					}
					auto pb = ServerPackets::Make_SC_HIT_PACKET(owner->GetTarget()->GetID(), targetHp);
					owner->GetGameRoom()->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));

					const auto fsm = GetFSM();

					static constexpr float ATTACK_PROB{ 0.6f };
					static std::default_random_engine dre{ std::random_device{}() };
					static std::uniform_real_distribution<float> dist{ 0.f, 1.f };
					if(dist(dre) <= ATTACK_PROB) {
						m_accDt = 0.f;
					}
					else {
						std::cout << "ATTACK -> DEFENSE" << std::endl;
						fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE, dt);
					}
					break;
				}
				default:
					break;
			}
		}
		// 타겟이 사라진 경우
		// -> 다시 IDLE 상태로 돌아간다.
		else {
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE, dt);
			return;
		}
	}
}

Server::Contents::SoldierDefenseState::SoldierDefenseState()
	:DefenseState{ FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE }
{

}

Server::Contents::SoldierDefenseState::~SoldierDefenseState()
{

}

void Server::Contents::SoldierDefenseState::Enter(const float dt)
{

}

void Server::Contents::SoldierDefenseState::Exit(const float dt)
{
	m_accDT = 0.f;
}

void Server::Contents::SoldierDefenseState::Update(const float dt)
{
	// TODO: Soldier Attack State Update
	m_accDT += dt;

	if(m_accDT >= static_cast<float>(DEFENSE_TIME.count())) {
		const auto fsm = GetFSM();

		static constexpr float ATTACK_PROB{ 0.6f };
		static std::default_random_engine dre{ std::random_device{}() };
		static std::uniform_real_distribution<float> dist{ 0.f, 1.f };
		if(dist(dre) <= ATTACK_PROB) {
			std::cout << "DEFENSE -> ATTACK" << std::endl;
			fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt);
		}
	}
}