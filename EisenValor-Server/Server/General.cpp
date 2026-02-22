#include "pch.h"
#include "General.h"

#include "GameWorld.h"
#include "Player.h"
#include "BehaviorTree.h"

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

	const auto& atkInfo{ GetAtkInfo() };
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

	auto pb{ ServerPackets::Make_SC_MOVE_PACKET(GetID(), GetPosInfo(), 0, 0) };
	GetGameWorld()->Broadcast(std::move(pb));
}

void Server::Contents::General::OnDeath()
{
	std::cout << "General::OnDeath()" << std::endl;
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DEAD, worldDT, true);

	// TODO: 블랙보드 초기화
}

void Server::Contents::General::OnRespawn()
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

	// TODO: General Respawn 시 부활 위치 설정 해야함.

	Vec3 pos{ GetPos() };
	pos.x += 10.f;
	pos.z += 10.f;
	SetPos(pos);

	auto pb{ ServerPackets::Make_SC_RESPAWN_GENERAL_PACKET(GetID(), GetPosInfo(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, GetStanceType()) };
	world->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
}

bool Server::Contents::General::OnDamaged(Creature* const attacker, const float dt)
{
	// TODO: 블랙보드에 공격자 정보 갱신
	auto const world{ GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };

	const auto fsm{ GetComponent<Server::Contents::FSM>() };
	const auto stateType{ fsm->GetCurState()->GetStateType() };

	if(FB_ENUMS::GENERAL_STATE_TYPE_DEAD == stateType)
		return false;

	const auto bt = GetComponent<BehaviorTree>();

	uint32 damage{};

	// 상대가 플레이어인 경우
	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {

		switch(stateType) {
			case FB_ENUMS::GENERAL_STATE_TYPE_ROAMING:
			{
				auto attackerPlayer = static_cast<Player*>(attacker);
				const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAtkInfo() };

				if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
					damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
				}
				else {
					damage = attackerAtkInfo.skillData->damage;
				}
				DecHP(damage);
				fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DUELING, dt, true);
				return true;
			}
			case FB_ENUMS::GENERAL_STATE_TYPE_DUELING:
			{
				// TODO: BT의 블랙보드에 저장된 정보 가져와서 
				// DUEL && def && 방향일치 -> 공격실패
		/*
			- 상대가 공격 O
				- 약공격 방어 확률 30%
				- 강공격 방어 확률 90%
				- 반격 확률 20%
			- 상대가 공격 X
				- 공격할 확률 60% (이때 스탠스를 바꿀 확률 50%)
				- 약공격 확률 40%
				- 약공격으로 시작하는 콤보 중 하나 실행확률 30%
				- 강공격 확률 50%
				- 페이크확률 60%
				- 강공격으로 시작하는 콤보 중 하나 실행확률 40%
				- 방어해제공격 10%
				- 움직이거나 스탠스만 변경할 확률 40%
*/

				break;
			}
			case FB_ENUMS::GENERAL_STATE_TYPE_STUN:
			{

				break;
			}
			default:
				break;
		}
	}
	return true;
}
