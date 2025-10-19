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
			const float detectionEnemyRangeSq = enemyDetectionRange * enemyDetectionRange;

			if(distSq <= detectionEnemyRangeSq) {
				std::static_pointer_cast<Creature>(owner)->SetTarget(target);
				fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_RUN, dt);
			}
		}
	}
}

Server::Contents::SoldierRunState::SoldierRunState()
	:RunState{ FB_ENUMS::SOLDIER_STATE_TYPE_RUN }
{
}

Server::Contents::SoldierRunState::~SoldierRunState()
{
}

void Server::Contents::SoldierRunState::Enter(const float dt)
{
	const auto& owner = GetFSM()->GetOwner();
	const uint32 id{ owner->GetID() };
	std::cout << std::format("ID = {}, Enter SoldierRunState", id) << std::endl;
}

void Server::Contents::SoldierRunState::Exit(const float dt)
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

		if(distToTarget < combatRange) {
			static constexpr float ATTACK_PROB{ 0.6f };
			static std::default_random_engine dre{ std::random_device{}() };
			static std::uniform_real_distribution<float> dist{ 0.f, 1.f };
			if(dist(dre) <= ATTACK_PROB) {
				std::cout << "RUN -> ATTACK" << std::endl;
				fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK, dt);
			}
			else {
				std::cout << "RUN -> DEFENSE" << std::endl;
				fsm->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_DEFENSE, dt);
			}
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


		// FIX: 주변에 적이 있는데도 불구하고 타겟이 없다고 판정되어 직진으로 가는 경우가 존재함.

		const float moveDist = 1.f * dt;
		const Vec3 dir = owner->GetForwardDir();
		Vec3 newPos{ ownerPos + dir * moveDist };
		owner->SetPos(newPos);

		auto otherTeamType = owner->GetGameRoom()->GetOtherTeamType(owner->GetTeamType());
		auto& otherTeam = owner->GetGameRoom()->GetTeam(otherTeamType);

		for(auto& [id, obj] : otherTeam.GetNpcs()) {
			const auto npc = std::static_pointer_cast<NPC>(obj);
			const auto npcPos = npc->GetPos();

			if(obj->GetNpcType() == FB_ENUMS::NPC_TYPE_SOLDIER) {
				const Vec3 toNpc = (npcPos - ownerPos);
				const float distToNpc = toNpc.Length();
				if(distToNpc < 0.5f) {
					owner->SetTarget(npc);
				}
			}
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

	if(m_accDt >= static_cast<float>(attackCycleTime.count())) {
		auto target = owner->GetTarget();
		if(target) {
			if(target->GetObjType() == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) {

			}
			else if(target->GetObjType() == FB_ENUMS::GAME_OBJECT_TYPE_NPC) {
				if(std::static_pointer_cast<NPC>(target)->GetNpcType() == FB_ENUMS::NPC_TYPE_SOLDIER) {
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
							target->GetGameRoom()->AddEvent([t = std::move(target)]()
								{
									// t->SetAlive(false);
									t->GetGameRoom()->GetTeam(t->GetTeamType()).RemoveObject(t);
								});
						}
						const auto fsm = GetFSM();
						// TODO: 일정 확률을 적용
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
				}
				else if(std::static_pointer_cast<NPC>(target)->GetNpcType() == FB_ENUMS::NPC_TYPE_GENERAL) {

				}
			}
		}
		else {
			GetFSM()->ChangeState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE, dt);
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