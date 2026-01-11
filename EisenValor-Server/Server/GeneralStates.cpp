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
	:State(GENERAL_STATE_TYPE::IDLE)
{
}

Server::Contents::GeneralIdleState::~GeneralIdleState()
{
}

void Server::Contents::GeneralIdleState::Enter(const float dt)
{
	m_accDTForStaminaRecovery = 0.f;
	std::cout << std::format("ID:{}, GeneralIdleState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralIdleState::Exit(const float dt)
{
	std::cout << std::format("ID:{}, GeneralIdleState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralIdleState::Update(const float dt)
{
	m_accDTForStaminaRecovery += dt;

	const General* const owner{ GetGeneral(GetFSM()) };
	if(owner->HasSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED)) {
		m_accDTForStaminaRecovery = 0.f;
		return;
	}

	if(m_accDTForStaminaRecovery >= std::chrono::duration<float>(std::chrono::seconds(1)).count()) {
		m_accDTForStaminaRecovery = 0.f;

		General* const owner{ GetGeneral(GetFSM()) };
		const auto& statInfo{ owner->GetStatInfo() };
		if(statInfo.stamina < statInfo.maxStamina) {
			owner->IncStamina(statInfo.staminaRecoveryPerSec);
			std::cout << std::format("General:{}, Stamina = {}", owner->GetID(), owner->GetStatInfo().stamina) << std::endl;
		}
	}
}

// ==================================
//		  GENERAL_MOVE_STATE
// ==================================
Server::Contents::GeneralMoveState::GeneralMoveState()
	:State(GENERAL_STATE_TYPE::MOVE)
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
	:State(GENERAL_STATE_TYPE::PRE_DELAY)
{
}

Server::Contents::GeneralPreDelayState::~GeneralPreDelayState()
{
}

void Server::Contents::GeneralPreDelayState::Enter(const float dt)
{
	General* const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
	std::cout << std::format("ID:{}, GeneralPreDelayState ENTER", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralPreDelayState::Exit(const float dt)
{
	std::cout << std::format("ID:{}, GeneralPreDelayState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralPreDelayState::Update(const float dt)
{
	General* const owner{ GetGeneral(GetFSM()) };

	const uint64 worldFrame{ owner->GetGameWorld()->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAttackInfo() };

	if(worldFrame >= m_startFrame + atkInfo.atkData->preDelayFrame) {
		GetFSM()->ChangeState(etou8(GENERAL_STATE_TYPE::ATTACK), dt);
	}
}

// ==================================
//		 GENERAL_ATTACK_STATE
// ==================================
Server::Contents::GeneralAttackState::GeneralAttackState()
	:State(GENERAL_STATE_TYPE::ATTACK)
{
}

Server::Contents::GeneralAttackState::~GeneralAttackState()
{
}

void Server::Contents::GeneralAttackState::Enter(const float dt)
{
	std::cout << std::format("ID:{}, GeneralAttackState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralAttackState::Exit(const float dt)
{
	std::cout << std::format("ID:{}, GeneralAttackState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralAttackState::Update(const float dt)
{
	General* const owner{ GetGeneral(GetFSM()) };
	const auto& atkInfo{ owner->GetAttackInfo() };
	if(auto const target = owner->GetTarget()) {
		if(owner->IsTargetInAttackRange(target)) {
			if(target->OnDamaged(owner, dt)) {
				owner->DecStamina(atkInfo.atkData->stamina);
				if(atkInfo.atkData->id == FB_ENUMS::GENERAL_ATTACK_TYPE_DISARM) {
					const FB_ENUMS::GAME_OBJECT_TYPE objType{ target->GetObjType() };
					if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == objType || FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == objType) {
						General* const obj{ static_cast<General*>(target) };
						if(auto const fsm{ target->GetComponent<Server::Contents::FSM>() }) {
							fsm->ChangeState(GENERAL_STATE_TYPE::IDLE, dt);
						}
					}
				}
			}

		}
	}			
	GetFSM()->ChangeState(GENERAL_STATE_TYPE::POST_DELAY, dt);
}

// ==================================
//		 GENERAL_POST_DELAY_STATE
// ==================================
Server::Contents::GeneralPostDelayState::GeneralPostDelayState()
	:State(GENERAL_STATE_TYPE::POST_DELAY)
{
}

Server::Contents::GeneralPostDelayState::~GeneralPostDelayState()
{
}

void Server::Contents::GeneralPostDelayState::Enter(const float dt)
{
	std::cout << std::format("ID:{}, GeneralPostDelayState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;
	General* const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
}

void Server::Contents::GeneralPostDelayState::Exit(const float dt)
{
	std::cout << std::format("ID:{}, GeneralPostDelayState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
}

void Server::Contents::GeneralPostDelayState::Update(const float dt)
{
	General* const owner{ GetGeneral(GetFSM()) };
	const uint64 worldFrame{ owner->GetGameWorld()->GetGameWorldFrameCount() };
	const auto& atkInfo{ owner->GetAttackInfo() };

	if(worldFrame >= m_startFrame + atkInfo.atkData->postDelayFrame) {
		GetFSM()->ChangeState(etou8(GENERAL_STATE_TYPE::IDLE), dt);
	}
}


// ==================================
//		 GENERAL_STUN_STATE
// ==================================
Server::Contents::GeneralStunState::GeneralStunState()
	:State(GENERAL_STATE_TYPE::STUN)
{
}

Server::Contents::GeneralStunState::~GeneralStunState()
{
}

void Server::Contents::GeneralStunState::Enter(const float dt)
{
	std::cout << std::format("ID:{}, GeneralStunState Enter", GetGeneral(GetFSM())->GetID()) << std::endl;

	General* const owner{ GetGeneral(GetFSM()) };
	m_startFrame = owner->GetGameWorld()->GetGameWorldFrameCount();
	if(m_stunDuration == 0) {
		m_stunDuration = owner->GetStatInfo().stunDelayFrame;
	}
}

void Server::Contents::GeneralStunState::Exit(const float dt)
{
	std::cout << std::format("ID:{}, GeneralStunState Exit", GetGeneral(GetFSM())->GetID()) << std::endl;
	General* const onwer{ GetGeneral(GetFSM()) };
	m_stunDuration = onwer->GetStatInfo().stunDelayFrame;
}

void Server::Contents::GeneralStunState::Update(const float dt)
{
	General* const onwer{ GetGeneral(GetFSM()) };
	const uint64 worldFrame{ onwer->GetGameWorld()->GetGameWorldFrameCount() };

	if(worldFrame >= m_startFrame + m_stunDuration) {
		GetFSM()->ChangeState(etou8(GENERAL_STATE_TYPE::IDLE), dt);
	}
}

// ==================================
//		 GENERAL_DEAD_STATE
// ==================================
Server::Contents::GeneralDeadState::GeneralDeadState()
	:State(GENERAL_STATE_TYPE::DEAD)
{
}

Server::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void Server::Contents::GeneralDeadState::Enter(const float dt)
{
}

void Server::Contents::GeneralDeadState::Exit(const float dt)
{
}

void Server::Contents::GeneralDeadState::Update(const float dt)
{
}

