#include "pch.h"
#include "GeneralStates.h"

#include "GameObject.h"
#include "General.h"
#include "BehaviorTree.h"
#include "GameWorld.h"
#include "BehaviorNode.h"
#include "GeneralNodes.h"
#include "NavAgent.h"

// #define PRINT_GENERAL_STATE_LOG

GameServer::Contents::GeneralState::GeneralState(const uint8 stateType)
	:State{stateType}
{
}

// ============================================
//					  IDLE
// ============================================
GameServer::Contents::GeneralIdleState::GeneralIdleState(const std::shared_ptr<General>& owner)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_IDLE}
{
	auto rootSelector = std::make_unique<GameServer::Contents::SelectorNode>();
	rootSelector->AddChild(std::make_unique<GameServer::Contents::IsTargetInNearRange>());
	rootSelector->AddChild(std::make_unique<GameServer::Contents::IsInOccupationZone>());
	rootSelector->AddChild(std::make_unique<GameServer::Contents::FindOZ>());

	rootSelector->SetOwner(owner);
	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralIdleState::~GeneralIdleState()
{
}

void GameServer::Contents::GeneralIdleState::Enter(const float dt)
{
	std::cout << "GeneralIdleState Enter!" << std::endl;
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralIdleState::Exit(const float dt)
{
}

void GameServer::Contents::GeneralIdleState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

// ===========================================
// 					  WALK
// ===========================================
GameServer::Contents::GeneralWalkState::GeneralWalkState(const std::shared_ptr<General>& owner)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_WALK}
{
	// TODO: WalkState의 행동 트리를 구성해야함.

	// 적이 있는지 확인
	// 적이 누군지 확인(플레이어인지, 장수인지, 병사인지)
	// 적이 플레이어거나 장수인 경우, 적과의 거리가 가까운지 확인
	// 적이 가까운 경우, 공격 시도
	// 적이 멀리 있는 경우, 적과의 거리를 좁히는 행동 시도

	// 점령지에 있는지 확인

	auto rootSelector = std::make_unique<GameServer::Contents::SelectorNode>();
	rootSelector->AddChild(std::make_unique<GameServer::Contents::IsTargetInNearRange>());

	rootSelector->SetOwner(owner);
	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralWalkState::~GeneralWalkState()
{
}

void GameServer::Contents::GeneralWalkState::Enter(const float dt)
{
	std::cout << "GeneralWalkState Enter!" << std::endl;
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralWalkState::Exit(const float dt)
{
	std::cout << "GeneralWalkState Exit!" << std::endl;
}

void GameServer::Contents::GeneralWalkState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

// ===========================================
// 					 RUN
// ===========================================
GameServer::Contents::GeneralRunState::GeneralRunState(const std::shared_ptr<General>& owner)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_RUN }
{
	auto rootSelector = std::make_unique<GameServer::Contents::SelectorNode>();
	rootSelector->AddChild(std::make_unique<GameServer::Contents::IsInOccupationZone>());
	rootSelector->AddChild(std::make_unique<GameServer::Contents::IsTargetInNearRange>());

	rootSelector->SetOwner(owner);
	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralRunState::~GeneralRunState()
{
}

void GameServer::Contents::GeneralRunState::Enter(const float dt)
{
	std::cout << "GeneralRunState Enter!" << std::endl;
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralRunState::Exit(const float dt)
{
	std::cout << "GeneralRunState Exit!" << std::endl;
}

void GameServer::Contents::GeneralRunState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

// ===========================================
// 					ATTACK
// ==========================================
GameServer::Contents::GeneralAttackState::GeneralAttackState()
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_ATTACK }
{
	// TODO: AttackState의 행동 트리를 구성해야함.
}

GameServer::Contents::GeneralAttackState::~GeneralAttackState()
{
}

void GameServer::Contents::GeneralAttackState::Enter(const float dt)
{
}

void GameServer::Contents::GeneralAttackState::Exit(const float dt)
{
}

void GameServer::Contents::GeneralAttackState::Update(const float dt)
{
}

// ===========================================
// 					STUN
// =========================================
GameServer::Contents::GeneralStunState::GeneralStunState()
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_STUN }
{
	// TODO: StunState의 행동 트리를 구성해야함.
}

GameServer::Contents::GeneralStunState::~GeneralStunState()
{
}

void GameServer::Contents::GeneralStunState::Enter(const float dt)
{
}

void GameServer::Contents::GeneralStunState::Exit(const float dt)
{
}

void GameServer::Contents::GeneralStunState::Update(const float dt)
{
}

// ===========================================
// 					DEAD
// =========================================
GameServer::Contents::GeneralDeadState::GeneralDeadState(const std::shared_ptr<General>& owner)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_DEAD }
{
	auto rootSequence = std::make_unique<GameServer::Contents::SequenceNode>();
	rootSequence->AddChild(std::make_unique<GameServer::Contents::IsRespawnReady>());
	rootSequence->AddChild(std::make_unique<GameServer::Contents::Respawn>());
	rootSequence->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));
	
	rootSequence->SetOwner(owner);
	m_root = std::move(rootSequence);
}

GameServer::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void GameServer::Contents::GeneralDeadState::Enter(const float dt)
{
	std::cout << "GeneralDeadState Enter!" << std::endl;
	const auto owner{ std::static_pointer_cast<General>(GetFSM()->GetOwner()) };
	owner->GetComponent<GameServer::Contents::NavAgent>()->StopMove();

	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralDeadState::Exit(const float dt)
{
	std::cout << "GeneralDeadState Exit!" << std::endl;
}

void GameServer::Contents::GeneralDeadState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}
