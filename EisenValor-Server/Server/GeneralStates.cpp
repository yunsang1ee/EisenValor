#include "pch.h"
#include "GeneralStates.h"

#include "GameObject.h"
#include "General.h"
#include "GameWorld.h"
#include "BehaviorNode.h"
#include "GeneralNodes.h"
#include "NavAgent.h"

#define PRINT_GENERAL_STATE_LOG

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

	// 0) 최초 생성 직후 2초 대기 (1회성, 이후 IDLE 재진입에는 영향 없음)
	rootSelector->AddChild(std::make_unique<GameServer::Contents::WaitOnce>(2.f));

	// 1) 적 감지 → 전투 진입 (Soldier 타겟이면 NEUTRAL, 그 외엔 COMBAT)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::FindEnemy>());
		seq->AddChild(std::make_unique<GameServer::Contents::SetStanceByTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_WALK));
		rootSelector->AddChild(std::move(seq));
	}

	// 2) 아직 점령되지 않은 점령지 안이면 그대로 대기 (점령 진행)
	rootSelector->AddChild(std::make_unique<GameServer::Contents::IsInUnoccupiedZone>());

	// 3) 모든 점령지가 점령된 상태에서 이미 어떤 점령지 안에 있으면 굳이 다른 점령지로
	//    옮길 필요가 없으므로 그대로 대기 (IDLE ↔ RUN flap 방지).
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::AreAllZonesOccupied>());
		seq->AddChild(std::make_unique<GameServer::Contents::IsInOccupationZone>());
		rootSelector->AddChild(std::move(seq));
	}

	// 4) 점령되지 않은 다른 점령지로 이동 (RUN 진입 전 기본 속도 복귀)
	//    - 점령지 밖이거나, 이미 점령된 점령지에 머무는 경우 모두 여기로 흐른다.
	//    - MoveToOZ가 점령되지 않은 점령지가 없으면 전체 점령지 중 랜덤을 선택한다.
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::SetMaxSpeed>(3.f));
		seq->AddChild(std::make_unique<GameServer::Contents::MoveToOZ>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_RUN));
		rootSelector->AddChild(std::move(seq));
	}

	rootSelector->SetOwner(owner);
	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralIdleState::~GeneralIdleState()
{
}

void GameServer::Contents::GeneralIdleState::Enter(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralIdleState Enter!" << std::endl;
#endif
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralIdleState::Exit(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralIdleState Exit!" << std::endl;
#endif
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
	auto rootSelector = std::make_unique<GameServer::Contents::SelectorNode>();

	// 1) 타겟 잃음 → IDLE 복귀
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetLost>());
		seq->AddChild(std::make_unique<GameServer::Contents::SetStance>(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL));
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));
		rootSelector->AddChild(std::move(seq));
	}

	// 2) 감지 범위 이탈 → 타겟 해제 후 IDLE
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		auto inv = std::make_unique<GameServer::Contents::InverterNode>();
		inv->SetChild(std::make_unique<GameServer::Contents::IsTargetInDetectionRange>());
		seq->AddChild(std::move(inv));
		seq->AddChild(std::make_unique<GameServer::Contents::ClearTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::SetStance>(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL));
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));
		rootSelector->AddChild(std::move(seq));
	}

	// 3a) 타겟이 Soldier + 공격 사거리 + 1.7s 쿨다운 ready → ATTACK 진입 (Soldier 사이클에 맞춘 빠른 교전)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetSoldier>());
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetInAttackRange>());
		seq->AddChild(std::make_unique<GameServer::Contents::IsAttackCooldownReady>(1.7f, 1.7f));
		seq->AddChild(std::make_unique<GameServer::Contents::StopMoving>());
		seq->AddChild(std::make_unique<GameServer::Contents::LookAtTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_ATTACK));
		rootSelector->AddChild(std::move(seq));
	}

	// 3b) (Soldier가 아닌) 공격 사거리 + 쿨다운 ready → ATTACK 진입(휘두름 한 프레임)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetInAttackRange>());
		seq->AddChild(std::make_unique<GameServer::Contents::IsAttackCooldownReady>());
		seq->AddChild(std::make_unique<GameServer::Contents::StopMoving>());
		seq->AddChild(std::make_unique<GameServer::Contents::LookAtTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_ATTACK));
		rootSelector->AddChild(std::move(seq));
	}

	// 3.5) 타겟이 Soldier + 공격 사거리 안 → 정지 + 응시 (쿨다운 대기, kiting 없이 Soldier처럼 교전)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetSoldier>());
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetInAttackRange>());
		seq->AddChild(std::make_unique<GameServer::Contents::StopMoving>());
		seq->AddChild(std::make_unique<GameServer::Contents::LookAtTarget>());
		rootSelector->AddChild(std::move(seq));
	}

	// 4) (Soldier가 아닌) 전투 범위 안 → 견제 무빙 + 방향 주기적 변경 (WALK 애니 유지, 속도 감속)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		auto inv = std::make_unique<GameServer::Contents::InverterNode>();
		inv->SetChild(std::make_unique<GameServer::Contents::IsTargetSoldier>());
		seq->AddChild(std::move(inv));
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetInCombatRange>());
		seq->AddChild(std::make_unique<GameServer::Contents::SetMaxSpeed>(2.f));
		seq->AddChild(std::make_unique<GameServer::Contents::LookAtTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::RandomizeAttackDir>());
		seq->AddChild(std::make_unique<GameServer::Contents::WanderAroundTarget>(1.2f, 1.0f, 2.5f));
		rootSelector->AddChild(std::move(seq));
	}

	// 5) 추격 (타겟 위치로 이동하며 응시, 기본 속도 복귀)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::SetMaxSpeed>(3.f));
		seq->AddChild(std::make_unique<GameServer::Contents::LookAtTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::MoveToTarget>());
		rootSelector->AddChild(std::move(seq));
	}

	rootSelector->SetOwner(owner);
	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralWalkState::~GeneralWalkState()
{
}

void GameServer::Contents::GeneralWalkState::Enter(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralWalkState Enter!" << std::endl;
#endif
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralWalkState::Exit(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralWalkState Exit!" << std::endl;
#endif
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

	// 1) 점령되지 않은 점령지 도착 → 정지 후 IDLE
	//    이미 점령된 점령지 안에서 RUN으로 진입한 경우, 그 zone을 빠져나갈 때까지 계속 RUN.
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::IsInUnoccupiedZone>());
		seq->AddChild(std::make_unique<GameServer::Contents::StopMoving>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));
		rootSelector->AddChild(std::move(seq));
	}

	// 1.5) 모든 점령지가 점령된 상태에서 임의 점령지 안에 도달 → 정지 후 IDLE
	//      (랜덤 점령지 fallback으로 진입한 경우의 도착 처리. IDLE에서 같은 조건의
	//      대기 분기가 다시 막아주므로 flap이 일어나지 않는다.)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::AreAllZonesOccupied>());
		seq->AddChild(std::make_unique<GameServer::Contents::IsInOccupationZone>());
		seq->AddChild(std::make_unique<GameServer::Contents::StopMoving>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));
		rootSelector->AddChild(std::move(seq));
	}

	// 2) 적 감지 → 전투 진입 (Soldier 타겟이면 NEUTRAL, 그 외엔 COMBAT)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::FindEnemy>());
		seq->AddChild(std::make_unique<GameServer::Contents::SetStanceByTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_WALK));
		rootSelector->AddChild(std::move(seq));
	}

	rootSelector->SetOwner(owner);
	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralRunState::~GeneralRunState()
{
}

void GameServer::Contents::GeneralRunState::Enter(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralRunState Enter!" << std::endl;
#endif
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralRunState::Exit(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralRunState Exit!" << std::endl;
#endif
}

void GameServer::Contents::GeneralRunState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

// ===========================================
// 					ATTACK
// ==========================================
GameServer::Contents::GeneralAttackState::GeneralAttackState(const std::shared_ptr<General>& owner)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_ATTACK }
{
	auto rootSelector = std::make_unique<GameServer::Contents::SelectorNode>();

	// 1) 타겟 상실 → IDLE 복귀
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::IsTargetLost>());
		seq->AddChild(std::make_unique<GameServer::Contents::ClearTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::SetStance>(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL));
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));
		rootSelector->AddChild(std::move(seq));
	}

	// 2) 감지 범위 이탈 → 타겟 해제 후 IDLE
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		auto inv = std::make_unique<GameServer::Contents::InverterNode>();
		inv->SetChild(std::make_unique<GameServer::Contents::IsTargetInDetectionRange>());
		seq->AddChild(std::move(inv));
		seq->AddChild(std::make_unique<GameServer::Contents::ClearTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::SetStance>(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL));
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));
		rootSelector->AddChild(std::move(seq));
	}

	// 3) 공격 사거리 이탈 → WALK로 복귀해 추격
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		auto inv = std::make_unique<GameServer::Contents::InverterNode>();
		inv->SetChild(std::make_unique<GameServer::Contents::IsTargetInAttackRange>());
		seq->AddChild(std::move(inv));
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_WALK));
		rootSelector->AddChild(std::move(seq));
	}

	// 4) 휘두름 한 프레임 (정지 → 응시 → 방향 갱신 → 공격 → WALK 복귀)
	{
		auto seq = std::make_unique<GameServer::Contents::SequenceNode>();
		seq->AddChild(std::make_unique<GameServer::Contents::StopMoving>());
		seq->AddChild(std::make_unique<GameServer::Contents::LookAtTarget>());
		seq->AddChild(std::make_unique<GameServer::Contents::Attack>());
		seq->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_WALK));
		rootSelector->AddChild(std::move(seq));
	}

	rootSelector->SetOwner(owner);
	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralAttackState::~GeneralAttackState()
{
}

void GameServer::Contents::GeneralAttackState::Enter(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
#endif
	std::cout << "GeneralAttackState Enter!" << std::endl;
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralAttackState::Exit(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
#endif
	std::cout << "GeneralAttackState Exit!" << std::endl;
}

void GameServer::Contents::GeneralAttackState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

// ===========================================
// 					STUN
// =========================================
GameServer::Contents::GeneralStunState::GeneralStunState(const std::shared_ptr<General>& owner)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_STUN }
{
	auto rootSequence = std::make_unique<GameServer::Contents::SequenceNode>();
	rootSequence->AddChild(std::make_unique<GameServer::Contents::IsStunOver>());
	rootSequence->AddChild(std::make_unique<GameServer::Contents::ChangeState>(FB_ENUMS::GENERAL_STATE_TYPE_IDLE));

	rootSequence->SetOwner(owner);
	m_root = std::move(rootSequence);
}

GameServer::Contents::GeneralStunState::~GeneralStunState()
{
}

void GameServer::Contents::GeneralStunState::Enter(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralStunState Enter!" << std::endl;
#endif
	const auto owner{ std::static_pointer_cast<General>(GetFSM()->GetOwner()) };
	owner->GetComponent<GameServer::Contents::NavAgent>()->StopMove();
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralStunState::Exit(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralStunState Exit!" << std::endl;
#endif
}

void GameServer::Contents::GeneralStunState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

// ===========================================
// 					DEAD
// =========================================
GameServer::Contents::GeneralDeadState::GeneralDeadState(const std::shared_ptr<General>& owner)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_DEAD }
{
	auto rootSequence = std::make_unique<GameServer::Contents::SequenceNode>();
	{
		auto onceStop = std::make_unique<GameServer::Contents::OnceNode>();
		onceStop->SetChild(std::make_unique<GameServer::Contents::StopMoving>());
		rootSequence->AddChild(std::move(onceStop));
	}
	{
		auto onceClear = std::make_unique<GameServer::Contents::OnceNode>();
		onceClear->SetChild(std::make_unique<GameServer::Contents::ClearTarget>());
		rootSequence->AddChild(std::move(onceClear));
	}
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
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralDeadState Enter!" << std::endl;
#endif
	if(m_root)
		m_root->Reset();
}

void GameServer::Contents::GeneralDeadState::Exit(const float dt)
{
#ifdef PRINT_GENERAL_STATE_LOG
	std::cout << "GeneralDeadState Exit!" << std::endl;
#endif
}

void GameServer::Contents::GeneralDeadState::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}
