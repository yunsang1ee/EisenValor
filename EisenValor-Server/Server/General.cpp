#include "pch.h"
#include "General.h"

#include "GameWorld.h"
#include "Player.h"
#include "NavAgent.h"
#include "Soldier.h"

// #define PRINT_GENERAL_LOG

GameServer::Contents::General::General(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE objType)
	:Creature(teamType, objType), m_stanceType{ FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL }, m_accDTForStaminaRecovery{}, m_accDTForRespawn{}
{
}

GameServer::Contents::General::~General()
{

}

bool GameServer::Contents::General::IsTargetInAttackRange(std::shared_ptr<GameObject> const target)
{
	if(!target) return false;

	const auto& atkInfo{ GetAtkInfo() };
	const float radiusSq{ atkInfo.skillData->attackRadius * atkInfo.skillData->attackRadius };
	const Vec3& myPos{ GetPosition() };

	const Vec3& myRot{ GetRotation() };
	Vec3 myDir{ sin(myRot.y), 0.f, cosf(myRot.y) };
	myDir.Normalize();

	const float halfDegree{ atkInfo.skillData->attackDegree * 0.5f };
	const float cosHalfAngle{ std::cosf(Deg2Rad(halfDegree)) };
	const Vec3& targetPos{ target->GetPosition() };
	const Vec3 toTargetDir{ targetPos - myPos };
	const float distToTargetSq = toTargetDir.x * toTargetDir.x + toTargetDir.y * toTargetDir.y + toTargetDir.z * toTargetDir.z;

	if(distToTargetSq >= radiusSq) {
		return false;
	}

	const float dotValue{ myDir.Dot(toTargetDir) };
	const float cosHalfAngleSq{ cosHalfAngle * cosHalfAngle };

	if(dotValue <= 0) {
		return false;
	}
	if((dotValue * dotValue >= distToTargetSq * cosHalfAngleSq)) {
		return true;
	}
	return false;
}

void GameServer::Contents::General::OnPostComponentUpdate(const float dt)
{
	if(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT == m_stanceType) return;

	const Vec3 delta = GetTransform().GetDeltaPosition();
	const float speedSq = delta.x * delta.x + delta.z * delta.z;

	if(speedSq > 0.0001f) {
		const Vec3 pos = GetPosition();
		LookAt({ pos.x + delta.x, pos.y, pos.z + delta.z });
	}
}

void GameServer::Contents::General::SetStanceType(const FB_ENUMS::GENERAL_STANCE_TYPE stanceType)
{
	if(m_stanceType == stanceType) return;

	m_stanceType = stanceType;

	auto pb = ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(GetID(), GetStanceType(), 0);
	GetGameWorld()->Broadcast(std::move(pb));
}

void GameServer::Contents::General::Update(const float dt)
{
	GameObject::Update(dt);

	const auto curMoveDir = GetMoveDir();
	const bool moveDirChanged = (curMoveDir != m_lastSentMoveDir);
	if(!ShouldBroadcastMove(dt, moveDirChanged))
		return;

	m_lastSentMoveDir = curMoveDir;
	auto pb{ ServerPackets::Make_SC_MOVE_PACKET(GetID(), GetTransform(), 0, curMoveDir) };
	GetGameWorld()->Broadcast(std::move(pb));
}

void GameServer::Contents::General::OnDeath()
{
#ifdef PRINT_GENERAL_LOG
	std::cout << "General::OnDeath()" << std::endl;
#endif
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	auto const fsm{ GetComponent<GameServer::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DEAD, worldDT, true);
}

void GameServer::Contents::General::OnRespawn()
{
	auto& statInfo{ GetStat() };
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	SetHp(statInfo.maxHP);
	SetStamina(statInfo.maxStamina);
	SetActive(true);
	SetTarget(nullptr);
	IncRespawnTime();
	SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	AddSubState(GENERAL_SUB_STATE_TYPE::NONE);
	
	GetComponent<GameServer::Contents::NavAgent>()->Teleport(m_respawnPos);
	ResetMoveBroadcastState();

	auto pb{ ServerPackets::Make_SC_RESPAWN_GENERAL_PACKET(GetID(), GetTransform(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, GetStanceType()) };
	world->Broadcast(std::move(pb));

#ifdef PRINT_GENERAL_LOG
	std::cout << "General NPC Respawn!" << std::endl;
#endif
}

bool GameServer::Contents::General::OnDamaged(std::shared_ptr<Creature> const attacker, const float dt, const bool broadcast)
{
	auto const world{ GetGameWorld() };

	const auto fsm{ GetComponent<GameServer::Contents::FSM>() };
	if(!fsm) return false;

	const auto stateType{ fsm->GetCurState()->GetStateType() };

	if(FB_ENUMS::GENERAL_STATE_TYPE_DEAD == stateType)
		return false;

	const auto atkInfo{ GetAtkInfo() };

	uint32 damage{};

	switch(const auto attackerType = attacker->GetObjType()) {
		case FB_ENUMS::GAME_OBJECT_TYPE_GENERAL:
		{
			auto attackGeneral{ std::static_pointer_cast<General>(attacker) };
			const AttackInfo& attackerAtkInfo{ attackGeneral->GetAtkInfo() };

			if(m_atkInfo.dir == attackerAtkInfo.dir) {
				auto pb{ ServerPackets::Make_SC_GENERAL_GUARD_PACKET(GetID(), attacker->GetID()) };
				world->Broadcast(std::move(pb));
				return false;
			}

			if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
				damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
			}
			else {
				damage = attackerAtkInfo.skillData->damage;
			}
			const uint32 currentHP{ DecHP(damage, broadcast) };
			if(0 == currentHP) {
				attacker->GetGameWorld()->AddScore(attacker->GetTeamType(), 2);
			}
			else {
				fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_STUN, dt, true);
			}
			break;
		}
		case FB_ENUMS::GAME_OBJECT_TYPE_PLAYER:
		{
			auto attackerPlayer = std::static_pointer_cast<Player>(attacker);
			const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAtkInfo() };

			if(m_atkInfo.dir == attackerAtkInfo.dir) {
				auto pb{ ServerPackets::Make_SC_GENERAL_GUARD_PACKET(GetID(), attacker->GetID()) };
				world->Broadcast(std::move(pb));
				return false;
			}

			if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
				damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
			}
			else {
				damage = attackerAtkInfo.skillData->damage;
			}
			const uint32 currentHP{ DecHP(damage, broadcast) };
			if(0 == currentHP) {
				attacker->GetGameWorld()->AddScore(attacker->GetTeamType(), 2);
			}
			else {
				fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_STUN, dt, true);
			}
			break;
		}
		case FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER:
		{
			auto attackerPlayer = std::static_pointer_cast<Soldier>(attacker);
			damage = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER)->atk;
			const uint32 currentHP{ DecHP(damage, broadcast) };
			if(0 != currentHP) {
				fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_STUN, dt, true);
			}


			break;
		}
		default:
			break;
	}

	if(damage > 0) {
		const auto world{ GetGameWorld() };
		auto pb{ ServerPackets::Make_SC_HIT_SOUND_PACKET(attacker->GetID()) };
		world->Broadcast(std::move(pb));
	}
	return true;
}
