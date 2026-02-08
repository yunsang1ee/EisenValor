#include "pch.h"
#include "General.h"

#include "GameWorld.h"

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
	const float cosHalfAngle{ std::cosf((degree) * (DirectX::XM_PI / 180.f)) };
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

void Server::Contents::General::OnDeath()
{
	std::cout << std::format("ID:{}, OnDeath!", GetID()) << std::endl;
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DEAD, worldDT);
}

void Server::Contents::General::Respawn()
{
	auto& statInfo{ GetStat() };
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	SetHp(statInfo.maxHP);
	SetStamina(statInfo.maxStamina);
	SetAlive(true);
	IncRespawnTime();
	SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	AddSubState(GENERAL_SUB_STATE_TYPE::NONE);

	Vec3 pos{ GetPos() };
	pos.x += 10.f;
	pos.z += 10.f;
	SetPos(pos);

	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE, worldDT);
	auto pb{ ServerPackets::Make_SC_RESPAWN_GENERAL_PACKET(GetID(), GetPosInfo(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, GetStanceType()) };
	world->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
}