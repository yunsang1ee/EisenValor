#include "pch.h"
#include "PlayerStates.h"

#include "GameWorld.h"
#include "General.h"
#include "FSM.h"

static std::shared_ptr<GameServer::Contents::General> GetGeneral(GameServer::Contents::FSM* fsm)
{
	return std::static_pointer_cast<GameServer::Contents::General>(fsm->GetOwner());
}

// ==================================
//		  GENERAL_IDLE_STATE
// ==================================
GameServer::Contents::PlayerIdleState::PlayerIdleState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), m_accDTForStaminaRecovery{}, m_accDTForExhaustedRecovery{}
{
}

GameServer::Contents::PlayerIdleState::~PlayerIdleState()
{
}

void GameServer::Contents::PlayerIdleState::Enter(const float dt)
{
	m_accDTForStaminaRecovery = 0.f;
	std::cout << "Enter Player Idle State" << std::endl;
}

void GameServer::Contents::PlayerIdleState::Exit(const float dt)
{
	m_accDTForStaminaRecovery = 0.f;
	m_accDTForExhaustedRecovery = 0.f;
	std::cout << "Exit Player Idle State" << std::endl;
}

void GameServer::Contents::PlayerIdleState::Update(const float dt)
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
		m_accDTForStaminaRecovery = 0.f;

		if(m_accDTForExhaustedRecovery >= 3.f) {
			owner->RemoveSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
			m_accDTForExhaustedRecovery = 0.f;
		}
		else {
			return;
		}
	}

	if(statInfo.currentStamina < statInfo.maxStamina) {
		m_accDTForStaminaRecovery += dt;

		if(m_accDTForStaminaRecovery >= 3.f) {
			m_accDTForStaminaRecovery = 0.F;

			owner->IncStamina(owner->GetGameObjectData()->staminaRecoveryPerSec);
		}
	}
	else {
		m_accDTForStaminaRecovery = 0.f;
	}
}

// ==================================
//		  GENERAL_MOVE_STATE
// ==================================
GameServer::Contents::PlayerMoveState::PlayerMoveState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_MOVE)
{

}

GameServer::Contents::PlayerMoveState::~PlayerMoveState()
{

}

void GameServer::Contents::PlayerMoveState::Enter(const float dt)
{
	std::cout << "Enter Player Move State" << std::endl;
}

void GameServer::Contents::PlayerMoveState::Exit(const float dt)
{
	std::cout << "Exit Player Move State" << std::endl;
}

void GameServer::Contents::PlayerMoveState::Update(const float dt)
{
	// TODO: General Move State
}

// ==================================
//		 GENERAL_PRE_DELAY_STATE
// ==================================
GameServer::Contents::PlayerPredelayState::PlayerPredelayState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY), m_startFrame{}
{
}

GameServer::Contents::PlayerPredelayState::~PlayerPredelayState()
{
}

void GameServer::Contents::PlayerPredelayState::Enter(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };

	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();

	std::cout << "Enter Player Predelay State" << std::endl;
}

void GameServer::Contents::PlayerPredelayState::Exit(const float dt)
{
	std::cout << "Exit Player Predelay State" << std::endl;
}

void GameServer::Contents::PlayerPredelayState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };

	const auto worldFrame{ owner->GetGameWorld()->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAtkInfo() };

	//if(worldFrame >= m_startFrame + atkInfo.skillData->preDelay) {
	//	auto const world{ owner->GetGameWorld() };
	//	auto const fsm{ owner->GetComponent<Server::Contents::FSM>() };
	//	fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK), dt, true);
	//}
	auto const world{ owner->GetGameWorld() };
	auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };
	fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK), dt, true);
}

// ==================================
//		 GENERAL_ATTACK_STATE
// ==================================
GameServer::Contents::PlayerAttackState::PlayerAttackState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK)
{
}

GameServer::Contents::PlayerAttackState::~PlayerAttackState()
{
}

void GameServer::Contents::PlayerAttackState::Enter(const float dt)
{
	std::cout << "Enter Player Attack State" << std::endl;
}

void GameServer::Contents::PlayerAttackState::Exit(const float dt)
{
	std::cout << "Exit Player Attack State" << std::endl;
}

void GameServer::Contents::PlayerAttackState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	const auto& atkInfo{ owner->GetAtkInfo() };
	auto const world{ owner->GetGameWorld() };

	if(false == IsValidObj(owner)) {
		auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY, dt, true);
		return;
	}

	auto const target = owner->GetTarget();

	if(false == IsValidObj(target)) {
		owner->SetTarget(nullptr);
		// std::cout << "PlayerAttackState! - false == IsValidObj(target)" << std::endl;

		auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY, dt, true);
		return;
	}

	// std::cout << std::format("PlayerAttackState!, Target ID: {}", target->GetID()) << std::endl;

	if(owner->IsTargetInAttackRange(target)) {
		
		if(false == IsValidObj(target)) {
			auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY, dt, true);
			return;
		}

		if(target->OnDamaged(owner, dt)) {

			if(atkInfo.skillData->skillTypeID == FB_ENUMS::GENERAL_ATTACK_TYPE_DISARM) {
				const FB_ENUMS::GAME_OBJECT_TYPE objType{ target->GetObjType() };
				// 무장해제 공격일 시, 상대 플레이어의 상태를 IDLE로...
				if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == objType) {
					auto const obj{ std::static_pointer_cast<General>(target) };
					auto const fsm{ obj->GetComponent<GameServer::Contents::FSM>() };
					fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE, dt, true);
					return;
				}
			}
		}
		else {
	//	std::cout << "Target OnDamaged Fail!" << std::endl;
		}
	}

	auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY, dt, true);
}

// ==================================
//		 GENERAL_POST_DELAY_STATE
// ==================================
GameServer::Contents::PlayerPostdelayState::PlayerPostdelayState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY), m_startFrame{}
{
}

GameServer::Contents::PlayerPostdelayState::~PlayerPostdelayState()
{
}

void GameServer::Contents::PlayerPostdelayState::Enter(const float dt)
{
	std::cout << "Enter Player Postdelay State" << std::endl;
	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
}

void GameServer::Contents::PlayerPostdelayState::Exit(const float dt)
{
	std::cout << "Exit Player Postdelay State" << std::endl;
}

void GameServer::Contents::PlayerPostdelayState::Update(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	auto const world{ owner->GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAtkInfo() };

	//if(worldFrame >= m_startFrame + atkInfo.skillData->postDelay) {
	//	auto const fsm{ GetFSM() };
	//	fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), dt, true);
	//}

	auto const fsm{ GetFSM() };
	fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), dt, true);
}

// ==================================
//		 GENERAL_STUN_STATE
// ==================================
GameServer::Contents::PlayerStunState::PlayerStunState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_STUN), m_startFrame{}, m_stunDuration{}
{
}

GameServer::Contents::PlayerStunState::~PlayerStunState()
{
}

void GameServer::Contents::PlayerStunState::Enter(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
	if(m_stunDuration == 0) {
		m_stunDuration = owner->GetGameObjectData()->stunDelay;
	}
	std::cout << "Enter Player Stun State" << std::endl;
}

void GameServer::Contents::PlayerStunState::Exit(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	m_stunDuration = owner->GetGameObjectData()->stunDelay;
	std::cout << "Exit Player Stun State" << std::endl;
}

void GameServer::Contents::PlayerStunState::Update(const float dt)
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
GameServer::Contents::PlayerDeadState::PlayerDeadState()
	:State(FB_ENUMS::PLAYER_STATE_TYPE_DEAD), m_accDTForRespawn{}
{
}

GameServer::Contents::PlayerDeadState::~PlayerDeadState()
{
}

void GameServer::Contents::PlayerDeadState::Enter(const float dt)
{
	m_accDTForRespawn = 0.f;
	std::cout << "Enter Player Dead State" << std::endl;
}

void GameServer::Contents::PlayerDeadState::Exit(const float dt)
{
	m_accDTForRespawn = 0.f;
	std::cout << "Exit Player Dead State" << std::endl;
}

void GameServer::Contents::PlayerDeadState::Update(const float dt)
{
	m_accDTForRespawn += dt;
	auto const owner{ GetGeneral(GetFSM()) };
	const auto& statInfo{ owner->GetStat() };

	if(m_accDTForRespawn >= statInfo.respawnTimeSec) {
		owner->OnRespawn();
	}
}

