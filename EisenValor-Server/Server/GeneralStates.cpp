#include "pch.h"
#include "GeneralStates.h"

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
Server::Contents::GeneralIdleState::GeneralIdleState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_IDLE), m_accDTForStaminaRecovery{}, m_accDTForExhaustedRecovery{}
{
}

Server::Contents::GeneralIdleState::~GeneralIdleState()
{
}

void Server::Contents::GeneralIdleState::Enter(const float dt)
{
	m_accDTForStaminaRecovery = 0.f;
	//std::cout << std::format("ID:{}, GeneralIdleState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralIdleState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralIdleState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
	m_accDTForStaminaRecovery = 0.f;
	m_accDTForExhaustedRecovery = 0.f;
}

void Server::Contents::GeneralIdleState::Update(const float dt)
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
Server::Contents::GeneralMoveState::GeneralMoveState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_MOVE)
{

}

Server::Contents::GeneralMoveState::~GeneralMoveState()
{

}

void Server::Contents::GeneralMoveState::Enter(const float dt)
{

}

void Server::Contents::GeneralMoveState::Exit(const float dt)
{

}

void Server::Contents::GeneralMoveState::Update(const float dt)
{
	// TODO: General Move State
}

// ==================================
//		 GENERAL_PRE_DELAY_STATE
// ==================================
Server::Contents::GeneralPreDelayState::GeneralPreDelayState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_PRE_DELAY), m_startFrame{}
{
}

Server::Contents::GeneralPreDelayState::~GeneralPreDelayState()
{
}

void Server::Contents::GeneralPreDelayState::Enter(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
//	std::cout << std::format("ID:{}, GeneralPreDelayState ENTER", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralPreDelayState::Exit(const float dt)
{
//	std::cout << std::format("ID:{}, GeneralPreDelayState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralPreDelayState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };

	const auto worldFrame{ owner->GetGameWorld()->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAttackInfo() };

	if(worldFrame >= m_startFrame + atkInfo.skillData->preDelay) {
		auto const world{ owner->GetGameWorld() };
		auto const fsm{ owner->GetComponent<Server::Contents::FSM>() };
		fsm->ChangeState(etou8(FB_ENUMS::GENERAL_STATE_TYPE_ATTACK), dt);
	}
}

// ==================================
//		 GENERAL_ATTACK_STATE
// ==================================
Server::Contents::GeneralAttackState::GeneralAttackState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_ATTACK)
{
}

Server::Contents::GeneralAttackState::~GeneralAttackState()
{
}

void Server::Contents::GeneralAttackState::Enter(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralAttackState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralAttackState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralAttackState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralAttackState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	const auto& atkInfo{ owner->GetAttackInfo() };
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
				if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == objType || FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == objType) {
					auto const obj{ static_cast<General*>(target) };
					auto const fsm{ obj->GetComponent<Server::Contents::FSM>() };
					fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE, dt);
				}
			}
		}
		else {
			std::cout << "Target OnDamaged Fail!" << std::endl;
		}
	}

	auto const fsm{ owner->GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_POST_DELAY, dt);
}

// ==================================
//		 GENERAL_POST_DELAY_STATE
// ==================================
Server::Contents::GeneralPostDelayState::GeneralPostDelayState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_POST_DELAY), m_startFrame{}
{
}

Server::Contents::GeneralPostDelayState::~GeneralPostDelayState()
{
}

void Server::Contents::GeneralPostDelayState::Enter(const float dt)
{
//	std::cout << std::format("ID:{}, GeneralPostDelayState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
}

void Server::Contents::GeneralPostDelayState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralPostDelayState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralPostDelayState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	auto const world{ owner->GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAttackInfo() };

	if(worldFrame >= m_startFrame + atkInfo.skillData->postDelay) {
		auto const fsm{ GetFSM() };
		fsm->ChangeState(etou8(FB_ENUMS::GENERAL_STATE_TYPE_IDLE), dt);
	}
}


// ==================================
//		 GENERAL_STUN_STATE
// ==================================
Server::Contents::GeneralStunState::GeneralStunState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_STUN), m_startFrame{}, m_stunDuration{}
{
}

Server::Contents::GeneralStunState::~GeneralStunState()
{
}

void Server::Contents::GeneralStunState::Enter(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralStunState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;

	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
	if(m_stunDuration == 0) {
		m_stunDuration = owner->GetGameObjectData()->stunDelay;
	}
}

void Server::Contents::GeneralStunState::Exit(const float dt)
{
//	std::cout << std::format("ID:{}, GeneralStunState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
	auto const owner{ GetGeneral(GetFSM()) };
	m_stunDuration = owner->GetGameObjectData()->stunDelay;
}

void Server::Contents::GeneralStunState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	auto const world{ owner->GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };
	auto const fsm{ GetFSM() };
	fsm->ChangeState(etou8(FB_ENUMS::GENERAL_STATE_TYPE_IDLE), dt);
}

// ==================================
//		 GENERAL_DEAD_STATE
// ==================================
Server::Contents::GeneralDeadState::GeneralDeadState()
	:State(FB_ENUMS::GENERAL_STATE_TYPE_DEAD), m_accDTForRespawn{}
{
}

Server::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void Server::Contents::GeneralDeadState::Enter(const float dt)
{
	m_accDTForRespawn = 0.f;
	//std::cout << std::format("ID:{}, GeneralDeadState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralDeadState::Exit(const float dt)
{
	//std::cout << std::format("ID:{}, GeneralDeadState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
	m_accDTForRespawn = 0.f;
}

void Server::Contents::GeneralDeadState::Update(const float dt)
{
	m_accDTForRespawn += dt;
	auto const owner{ GetGeneral(GetFSM()) };
	const auto& statInfo{ owner->GetStat() };

	if(m_accDTForRespawn >= statInfo.respawnTimeSec) {
		owner->Respawn();
	}
}

