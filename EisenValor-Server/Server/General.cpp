#include "pch.h"
#include "General.h"

#include "GameWorld.h"
#include "Player.h"


Server::Contents::General::General(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE objType)
	:Creature(teamType, objType), m_stanceType{ FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL }, m_accDTForStaminaRecovery{}, m_accDTForRespawn{}
{
}

Server::Contents::General::~General()
{

}

bool Server::Contents::General::IsTargetInAttackRange(GameObject* const target)
{
	if(!target) return false;

	const auto& atkInfo{ GetAttackInfo() };
	const float radiusSq{ atkInfo.skillData->attackRadius * atkInfo.skillData->attackRadius };
	const Vec3& myPos{ GetPos() };

	const Vec3& myRot{ GetRotation() };
	Vec3 myDir{ sinf(Deg2Rad(myRot.y)), 0.f, cosf(Deg2Rad(myRot.y)) };
	myDir.Normalize();

	const float degree{ atkInfo.skillData->attackDegree * 0.5f };
	const float cosHalfAngle{ std::cosf(Deg2Rad(degree)) };
	const Vec3& targetPos{ target->GetPos() };
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

void Server::Contents::General::Update(const float dt)
{
	GameObject::Update(dt);
}

void Server::Contents::General::OnDeath()
{
	std::cout << "General::OnDeath()" << std::endl;
}

void Server::Contents::General::Respawn()
{
	auto& statInfo{ GetStat() };
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	SetHp(statInfo.maxHP);
	SetStamina(statInfo.maxStamina);
	SetActive(true);
	IncRespawnTime();
	SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	AddSubState(GENERAL_SUB_STATE_TYPE::NONE);

	Vec3 pos{ GetPos() };
	pos.x += 10.f;
	pos.z += 10.f;
	SetPos(pos);

	auto pb{ ServerPackets::Make_SC_RESPAWN_GENERAL_PACKET(GetID(), GetPosInfo(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, GetStanceType()) };
	world->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
}

bool Server::Contents::General::OnDamaged(Creature* const attacker, const float dt)
{
	auto const world{ GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };

	const auto fsm{ GetComponent<Server::Contents::FSM>() };
	const auto stateType{ fsm->GetCurState()->GetStateType() };

	uint32 damage{};

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {
		auto attackerPlayer = static_cast<Player*>(attacker);
		const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAttackInfo() };

		if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
			damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
		}
		else {
			damage = attackerAtkInfo.skillData->damage;
		}
	}

	DecHP(damage);

	return true;

}
