#include "pch.h"
#include "PlayerStates.h"

#include "GameWorld.h"
#include "General.h"
#include "FSM.h"

static Server::Contents::General* GetGeneral(Server::Contents::FSM* fsm)
{
	return static_cast<Server::Contents::General*>(fsm->GetOwner());
}

// ==================================
//		  GENERAL_IDLE_STATE
// ==================================
Server::Contents::PlayerIdleState::PlayerIdleState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), m_accDTForStaminaRecovery{}, m_accDTForExhaustedRecovery{}
{
}

Server::Contents::PlayerIdleState::~PlayerIdleState()
{
}

void Server::Contents::PlayerIdleState::Enter(const float dt)
{
	m_accDTForStaminaRecovery = 0.f;
	//std::cout << std::format("ID:{}, GeneralIdleState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::PlayerIdleState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralIdleState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
	m_accDTForStaminaRecovery = 0.f;
	m_accDTForExhaustedRecovery = 0.f;
}

void Server::Contents::PlayerIdleState::Update(const float dt)
{
	//m_accDTForStaminaRecovery += dt;
	//auto const owner{ GetGeneral(GetFSM()) };
	//if(owner->HasSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED)) {
	//	m_accDTForExhaustedRecovery += dt;
	//	m_accDTForStaminaRecovery = 0.f;
	//	if(m_accDTForExhaustedRecovery >= 3.f) {
	//		owner->RemoveSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
	//		m_accDTForExhaustedRecovery = 0.f;
	//		m_accDTForStaminaRecovery = 0.f;
	//	}
	//	return;
	//}
	//else {
	//	if(m_accDTForStaminaRecovery >= 1.f) {
	//		m_accDTForStaminaRecovery = 0.f;
	//		const auto& statInfo{ owner->GetStatInfo() };
	//		if(statInfo.currentStamina < statInfo.maxStamina) {
	//			owner->IncStamina(statInfo.staminaRecoveryPerSec);
	//			auto pb{ ServerPackets::Make_SC_UPDATE_VITAL_PACKET(owner->GetID(), owner->GetHP(), owner->GetStamina()) };
	//			owner->GetGameWorld()->Broadcast(std::move(pb));
	//		}
	//	}
	//}

	auto const owner = GetGeneral(GetFSM());
	const auto& statInfo = owner->GetStat();

	// 1. 탈진 상태 관리
	if(owner->HasSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED)) {
		m_accDTForExhaustedRecovery += dt;
		m_accDTForStaminaRecovery = 0.f; // 탈진 중에는 회복 타이머 정지

		if(m_accDTForExhaustedRecovery >= 3.f) {
			owner->RemoveSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
			m_accDTForExhaustedRecovery = 0.f;
			// 탈진이 풀렸으므로 즉시 일반 로직으로 이어지게 return 생략 가능
		}
		else {
			return; // 아직 탈진 중이면 여기서 종료
		}
	}

	// 2. 스태미나 회복 체크 (최대치라면 연산/브로드캐스트 생략)
	if(statInfo.currentStamina < statInfo.maxStamina) {
		m_accDTForStaminaRecovery += dt;

		if(m_accDTForStaminaRecovery >= 3.f) {
			// 오차 보정: 1.0을 빼서 남은 시간을 다음 턴으로 이월
			m_accDTForStaminaRecovery = 0.F;

			owner->IncStamina(owner->GetGameObjectData()->staminaRecoveryPerSec);
		}
	}
	else {
		// 이미 가득 찼다면 타이머 초기화 (나중에 다시 닳았을 때 1초 뒤부터 회복되도록)
		m_accDTForStaminaRecovery = 0.f;
	}
}

// ==================================
//		  GENERAL_MOVE_STATE
// ==================================
Server::Contents::PlayerMoveState::PlayerMoveState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_MOVE)
{

}

Server::Contents::PlayerMoveState::~PlayerMoveState()
{

}

void Server::Contents::PlayerMoveState::Enter(const float dt)
{

}

void Server::Contents::PlayerMoveState::Exit(const float dt)
{

}

void Server::Contents::PlayerMoveState::Update(const float dt)
{
	// TODO: General Move State
}

// ==================================
//		 GENERAL_PRE_DELAY_STATE
// ==================================
Server::Contents::PlayerPredelayState::PlayerPredelayState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY), m_startFrame{}
{
}

Server::Contents::PlayerPredelayState::~PlayerPredelayState()
{
}

void Server::Contents::PlayerPredelayState::Enter(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
	//	std::cout << std::format("ID:{}, GeneralPreDelayState ENTER", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::PlayerPredelayState::Exit(const float dt)
{
	//	std::cout << std::format("ID:{}, GeneralPreDelayState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::PlayerPredelayState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };

	const auto worldFrame{ owner->GetGameWorld()->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAtkInfo() };

	if(worldFrame >= m_startFrame + atkInfo.skillData->preDelay) {
		auto const world{ owner->GetGameWorld() };
		auto const fsm{ owner->GetComponent<Server::Contents::FSM>() };
		fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK), dt, true);
	}
}

// ==================================
//		 GENERAL_ATTACK_STATE
// ==================================
Server::Contents::PlayerAttackState::PlayerAttackState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK)
{
}

Server::Contents::PlayerAttackState::~PlayerAttackState()
{
}

void Server::Contents::PlayerAttackState::Enter(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralAttackState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::PlayerAttackState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralAttackState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::PlayerAttackState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	const auto& atkInfo{ owner->GetAtkInfo() };
	auto const world{ owner->GetGameWorld() };

	auto const target = owner->GetTarget();
	if(!target) {
		return;
	}

	if(owner->IsTargetInAttackRange(target)) {
		if(nullptr == target) return;

		if(target->OnDamaged(owner, dt)) {

			if(atkInfo.skillData->skillTypeID == FB_ENUMS::GENERAL_ATTACK_TYPE_DISARM) {
				const FB_ENUMS::GAME_OBJECT_TYPE objType{ target->GetObjType() };
				// 무장해제 공격일 시, 상대 플레이어의 상태를 IDLE로...
				if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == objType) {
					auto const obj{ static_cast<General*>(target) };
					auto const fsm{ obj->GetComponent<Server::Contents::FSM>() };
					fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE, dt, true);
				}
			}
		}
		else {
			std::cout << "Target OnDamaged Fail!" << std::endl;
		}
	}

	auto const fsm{ owner->GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY, dt, true);
}

// ==================================
//		 GENERAL_POST_DELAY_STATE
// ==================================
Server::Contents::PlayerPostdelayState::PlayerPostdelayState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY), m_startFrame{}
{
}

Server::Contents::PlayerPostdelayState::~PlayerPostdelayState()
{
}

void Server::Contents::PlayerPostdelayState::Enter(const float dt)
{
	//	std::cout << std::format("ID:{}, GeneralPostDelayState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
}

void Server::Contents::PlayerPostdelayState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralPostDelayState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::PlayerPostdelayState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	auto const world{ owner->GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAtkInfo() };

	if(worldFrame >= m_startFrame + atkInfo.skillData->postDelay) {
		auto const fsm{ GetFSM() };
		fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), dt, true);
	}
}


// ==================================
//		 GENERAL_STUN_STATE
// ==================================
Server::Contents::PlayerStunState::PlayerStunState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_STUN), m_startFrame{}, m_stunDuration{}
{
}

Server::Contents::PlayerStunState::~PlayerStunState()
{
}

void Server::Contents::PlayerStunState::Enter(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralStunState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;

	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
	if(m_stunDuration == 0) {
		m_stunDuration = owner->GetGameObjectData()->stunDelay;
	}
}

void Server::Contents::PlayerStunState::Exit(const float dt)
{
	//	std::cout << std::format("ID:{}, GeneralStunState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
	auto const owner{ GetGeneral(GetFSM()) };
	m_stunDuration = owner->GetGameObjectData()->stunDelay;
}

void Server::Contents::PlayerStunState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	auto const world{ owner->GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };
	auto const fsm{ GetFSM() };
	fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), dt, true);
}

// ==================================
//		 GENERAL_DEAD_STATE
// ==================================
Server::Contents::PlayerDeadState::PlayerDeadState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_DEAD), m_accDTForRespawn{}
{
}

Server::Contents::PlayerDeadState::~PlayerDeadState()
{
}

void Server::Contents::PlayerDeadState::Enter(const float dt)
{
	m_accDTForRespawn = 0.f;
	//std::cout << std::format("ID:{}, GeneralDeadState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::PlayerDeadState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralDeadState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
	m_accDTForRespawn = 0.f;
}

void Server::Contents::PlayerDeadState::Update(const float dt)
{
	m_accDTForRespawn += dt;
	auto const owner{ GetGeneral(GetFSM()) };
	const auto& statInfo{ owner->GetStat() };

	if(m_accDTForRespawn >= statInfo.respawnTimeSec) {
		owner->OnRespawn();
	}
}

