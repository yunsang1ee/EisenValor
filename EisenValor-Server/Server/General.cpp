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
	//if(!target) return false;

	//const auto& atkInfo{ GetAttackInfo() };
	//const float radiusSq{ atkInfo.skillData->attackRadius * atkInfo.skillData->attackRadius };
	//const Vec3& myPos{ GetPos() };
	//const Vec3& myRot{ GetRotation() };
	//Vec3 myDir{ sinf(myRot.y), 0.f, cosf(myRot.y) };
	//myDir.Normalize();

	//const float cosHalfAngle{ std::cosf((atkInfo.skillData->attackDegree * 0.5f) * DirectX::XM_PI / 180.f) };
	//const Vec3& targetPos{ target->GetPos() };
	//const Vec3 toTargetDir{ targetPos - myPos };
	//const float distToTargetSq = toTargetDir.x * toTargetDir.x + toTargetDir.y * toTargetDir.y + toTargetDir.z * toTargetDir.z;

	//if(distToTargetSq >= radiusSq) return false;

	//const float dotValue{ myDir.Dot(toTargetDir) };
	//const float cosHalfAngleSq{ cosHalfAngle * cosHalfAngle };

	////		// a * b = |a| |b| cos	
	//// cos = a * b / |a| |b|
	//// 공격 판정 -> theta <= halfAngle -> cos(theta) >= cos(halfAngle)
	//// dotValue < 0 -> (즉, 플레이어가 바라보는 반대편)인 경우에도, 제곱하면 양수가 된다 -> 뒤쪽 NPC가 공격 맞은것처럼 판정될 수 있음.
	//if(dotValue <= 0) return false;
	//if((dotValue * dotValue >= distToTargetSq * cosHalfAngleSq))
	//	return true;

	//return false;

	if(!target) return false;

	const auto& atkInfo{ GetAttackInfo() };
	const Vec3& myPos{ GetPos() };
	const Vec3& targetPos{ target->GetPos() };

	// 1. 거리 체크 (y축을 제외할지 결정 필요)
	Vec3 toTarget = targetPos - myPos;
	toTarget.y = 0.f; // 높이 차이를 무시하려면 활성화
	float distSq = toTarget.LengthSquared();
	float range = atkInfo.skillData->attackRadius;

	if(distSq > range * range) {
		std::cout << "distSq > range * range" << std::endl;
		return false;
	}

	// 2. 각도 체크
	const Vec3& myRot{ GetRotation() };
	// 이미 단위 벡터이므로 Normalize 불필요
	Vec3 myDir{ sinf(myRot.y), 0.f, cosf(myRot.y) };
	myDir.Normalize();

	// 내적을 위해 대상 방향 벡터를 정규화 (혹은 dot / dist 로 계산)
	float dist = std::sqrt(distSq);
	if(dist < 0.0001f) {
		std::cout << "dist < 0.0001f" << std::endl;
		return true; // 거의 겹쳐 있으면 히트
	}

	float dot = (toTarget.x * myDir.x + toTarget.z * myDir.z) / dist;

	// Degree를 라디안으로 바꾼 뒤 코사인 값 미리 계산 (캐싱 권장)
	float cosHalfAngle = std::cosf((atkInfo.skillData->attackDegree * 0.5f) * DirectX::XM_PI / 180.f);

	// 내적값(cos theta)이 기준 코사인값보다 크면 범위 안임
	return dot >= cosHalfAngle;
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