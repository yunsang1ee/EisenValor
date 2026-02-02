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
	const auto& atkInfo{ GetAttackInfo() };
	const float radiusSq{ atkInfo.atkData->attackRadius * atkInfo.atkData->attackRadius };
	const Vec3& myPos{ GetPos() };
	const Vec3& myRot{ GetRotation() };
	Vec3 myDir{ sinf(myRot.y), 0.f, cosf(myRot.y) };
	myDir.Normalize();
	const float cosHalfAngle{ std::cosf((atkInfo.atkData->attackDegree * 0.5f) * DirectX::XM_PI / 180.f) };
	if(target){
		const Vec3& targetPos{ target->GetPos() };
		const Vec3 toTargetDir{ targetPos - myPos };
		const float distToTargetSq = toTargetDir.x * toTargetDir.x + toTargetDir.y * toTargetDir.y + toTargetDir.z * toTargetDir.z;
		if(distToTargetSq >= radiusSq) return false;
		const float dotValue{ myDir.Dot(toTargetDir) };
		const float cosHalfAngleSq{ cosHalfAngle * cosHalfAngle };
		//		// a * b = |a| |b| cos	
		// cos = a * b / |a| |b|
		// 공격 판정 -> theta <= halfAngle -> cos(theta) >= cos(halfAngle)
		// dotValue < 0 -> (즉, 플레이어가 바라보는 반대편)인 경우에도, 제곱하면 양수가 된다 -> 뒤쪽 NPC가 공격 맞은것처럼 판정될 수 있음.
		if(dotValue <= 0) return false;
		if((dotValue * dotValue >= distToTargetSq * cosHalfAngleSq))
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
	auto& statInfo{ GetStatInfo() };
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	SetHp(statInfo.maxHP);
	SetStamina(statInfo.maxStamina);
	SetAlive(true);
	IncRespawnTime();
	
	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE, worldDT);
	auto pb{ ServerPackets::Make_SC_RESPAWN_OBJECT_PACKET(GetID(), GetPosInfo(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina) };
	world->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
}