#include "pch.h"
#include "PlayerStates.h"

#include "GameWorld.h"
#include "General.h"
#include "FSM.h"

// #define PRINT_PLAYER_STATE_LOG

static std::shared_ptr<GameServer::Contents::General> GetGeneral(GameServer::Contents::FSM* fsm)
{
	return std::static_pointer_cast<GameServer::Contents::General>(fsm->GetOwner());
}

// ==================================
//		  GENERAL_IDLE_STATE
// ==================================
GameServer::Contents::PlayerIdleState::PlayerIdleState()
	:State{ FB_ENUMS::PLAYER_STATE_TYPE_IDLE }, m_accDTForStaminaRecovery{}, m_accDTForExhaustedRecovery{}
{
}

GameServer::Contents::PlayerIdleState::~PlayerIdleState()
{
}

void GameServer::Contents::PlayerIdleState::Enter(const float dt)
{
	m_accDTForStaminaRecovery = 0.f;
	m_accDTForExhaustedRecovery = 0.f;

#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Enter Player Idle State" << std::endl;
#endif
}

void GameServer::Contents::PlayerIdleState::Exit(const float dt)
{
	m_accDTForStaminaRecovery = 0.f;
	m_accDTForExhaustedRecovery = 0.f;

#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Exit Player Idle State" << std::endl;
#endif
}

void GameServer::Contents::PlayerIdleState::Update(const float dt)
{
	const auto& owner{ GetGeneral(GetFSM()) };
	const auto& statInfo{ owner->GetStat() };

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
// 		 PLAYER_WALK_STATE
// ==================================
GameServer::Contents::PlayerWalkState::PlayerWalkState()
	:State{ FB_ENUMS::PLAYER_STATE_TYPE_WALK }
{
}

GameServer::Contents::PlayerWalkState::~PlayerWalkState()
{
}

void GameServer::Contents::PlayerWalkState::Enter(const float dt)
{
}

void GameServer::Contents::PlayerWalkState::Exit(const float dt)
{
}

void GameServer::Contents::PlayerWalkState::Update(const float dt)
{
}

// ==================================
// 		 PLAYER_RUN_STATE
// ==================================
GameServer::Contents::PlayerRunState::PlayerRunState()
	:State{ FB_ENUMS::PLAYER_STATE_TYPE_RUN }
{
}

GameServer::Contents::PlayerRunState::~PlayerRunState()
{
}

void GameServer::Contents::PlayerRunState::Enter(const float dt)
{
}

void GameServer::Contents::PlayerRunState::Exit(const float dt)
{
}

void GameServer::Contents::PlayerRunState::Update(const float dt)
{
}

// ==================================
//		 PLAYER_PRE_DELAY_STATE
// ==================================
GameServer::Contents::PlayerPredelayState::PlayerPredelayState()
	:State{ FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY }, m_accDTForPreDelay{ 0.f }
{
}

GameServer::Contents::PlayerPredelayState::~PlayerPredelayState()
{
}

void GameServer::Contents::PlayerPredelayState::Enter(const float dt)
{
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Enter Player Predelay State" << std::endl;
#endif
	m_accDTForPreDelay = 0.f;
}

void GameServer::Contents::PlayerPredelayState::Exit(const float dt)
{
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Exit Player Predelay State" << std::endl;
#endif
}

void GameServer::Contents::PlayerPredelayState::Update(const float dt)
{
	const auto& owner{ GetGeneral(GetFSM()) };

	m_accDTForPreDelay += dt;

	const auto& atkInfo{ owner->GetAtkInfo() };
	auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };

	if(atkInfo.skillData->name == "LIGHT") {
		if(m_accDTForPreDelay >= 0.3f) {
			std::cout << "PlayerPredelayState Update, LIGHT Attack PreDelay End!" << std::endl;
			fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK), dt, true);
		}
	}
	else {
		if(m_accDTForPreDelay >= 0.6f) {
			std::cout << "PlayerPredelayState Update, HEAVY Attack PreDelay End!" << std::endl;
			fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK), dt, true);
		}
	}
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
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Enter Player Attack State" << std::endl;
#endif

	m_accDT = 0.f;      
	m_hitFired = false; 
}	

void GameServer::Contents::PlayerAttackState::Exit(const float dt)
{
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Exit Player Attack State" << std::endl;
#endif

	m_accDT = 0.f;    
	m_hitFired = false;
}

void GameServer::Contents::PlayerAttackState::Update(const float dt)
{
	float HIT_FRAME_DELAY{ 0.1f };

	const auto& owner{ GetGeneral(GetFSM()) };
	const auto& atkInfo{ owner->GetAtkInfo() };
	auto const world{ owner->GetGameWorld() };

	if(etou8(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT) == owner->GetAtkInfo().skillData->skillTypeID) {
		HIT_FRAME_DELAY = 0.4f;
	}
	else if(etou8(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY) == owner->GetAtkInfo().skillData->skillTypeID ) {
		HIT_FRAME_DELAY = 0.6f;
	}
	else
		HIT_FRAME_DELAY = 0.1f;

	m_accDT += dt;                       

	if(m_accDT < HIT_FRAME_DELAY) return;
	if(m_hitFired) return;               
	m_hitFired = true;						

	if(false == IsValidObj(owner)) return;

	const auto ownerStanceType{ owner->GetStanceType() };	

	switch(ownerStanceType) {
		case FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL:
		{
			const auto& gameObjects = world->GetGameObjectGroups();

			for(int i = 0; i < FB_ENUMS::GAME_OBJECT_TYPE_END; ++i) {
				if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER && i != FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER)
					continue;

				for(const auto& [id, o] : gameObjects[i]) {
					if(false == IsValidObj(o))
						continue;
					if(owner->GetID() == id)
						continue;
					if(o->GetTeamType() == owner->GetTeamType()) continue;
					if(owner->IsTargetInAttackRange(o)) {
						const auto target = static_cast<Creature*>(o.get()); 
						target->OnDamaged(owner, dt);
					}
				}
			}
			break;
		}
		case FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT:
		{
			const auto& gameObjects = world->GetGameObjectGroups();

			for(int i = 0; i < FB_ENUMS::GAME_OBJECT_TYPE_END; ++i) {
				if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER && i != FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER)
					continue;

				for(const auto& [id, o] : gameObjects[i]) {
					if(false == IsValidObj(o))
						continue;
					if(owner->GetID() == id)
						continue;
					if(o->GetTeamType() == owner->GetTeamType()) continue;
					if(owner->IsTargetInAttackRange(o)) {
						owner->SetTarget(std::static_pointer_cast<Creature>(o));
						break;
					}
				}
			}

			const auto target{ owner->GetTarget() };

			if(false == IsValidObj(target)) {
				owner->SetTarget(nullptr);
				auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };
				fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY, dt, true);
				return;
			}

			std::cout << std::format("PlayerAttackState!, Target ID: {}", target->GetID()) << std::endl;

			if(owner->IsTargetInAttackRange(target)) {
				if(target->OnDamaged(owner, dt)) {

					if(atkInfo.skillData->skillTypeID == etou8(FB_ENUMS::GENERAL_ATTACK_TYPE_DISARM)) {
						const FB_ENUMS::GAME_OBJECT_TYPE objType{ target->GetObjType() };
						// 무장해제 공격일 시, 상대 플레이어나 장수의 상태를 IDLE로...
						if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == objType || FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == objType) {
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
			break;
		}
		default:
			break;
	}

	auto const fsm{ owner->GetComponent<GameServer::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY, dt, true);
}

// ==================================
//		 PLAYER_POST_DELAY_STATE
// ==================================
GameServer::Contents::PlayerPostdelayState::PlayerPostdelayState()
	:State{ FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY }
{
}

GameServer::Contents::PlayerPostdelayState::~PlayerPostdelayState()
{
}

void GameServer::Contents::PlayerPostdelayState::Enter(const float dt)
{
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Enter Player Postdelay State" << std::endl;
#endif
}

void GameServer::Contents::PlayerPostdelayState::Exit(const float dt)
{
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Exit Player Postdelay State" << std::endl;
#endif
}

void GameServer::Contents::PlayerPostdelayState::Update(const float dt)
{
	auto const fsm{ GetFSM() };
	fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), dt, true);
}

// ==================================
//		 PLAYER_STUN_STATE
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
	//m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
	if(m_stunDuration == 0) {
		m_stunDuration = owner->GetGameObjectData()->stunDelay;
	}
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Enter Player Stun State" << std::endl;
#endif
}

void GameServer::Contents::PlayerStunState::Exit(const float dt)
{
	auto const owner{ GetGeneral(GetFSM()) };
	m_stunDuration = owner->GetGameObjectData()->stunDelay;
#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Exit Player Stun State" << std::endl;
#endif
}

void GameServer::Contents::PlayerStunState::Update(const float dt)
{
	auto const fsm{ GetFSM() };
	fsm->ChangeState(etou8(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), dt, true);
}

// ==================================
//		 PLAYER_DEAD_STATE
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

#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Enter Player Dead State" << std::endl;
#endif
}

void GameServer::Contents::PlayerDeadState::Exit(const float dt)
{
	m_accDTForRespawn = 0.f;

#ifdef PRINT_PLAYER_STATE_LOG
	std::cout << "Exit Player Dead State" << std::endl;
#endif
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