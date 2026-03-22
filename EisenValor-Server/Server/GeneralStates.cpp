#include "pch.h"
#include "GeneralStates.h"

#include "GameObject.h"
#include "General.h"
#include "BehaviorTree.h"
#include "GameWorld.h"
#include "BehaviorNode.h"
#include "GeneralNodes.h"
#include "NavAgent.h"

GameServer::Contents::GeneralState::GeneralState(const uint8 stateType, FSM* const fsm)
	:State{stateType}
{
	SetFSM(fsm);

	m_owner = std::static_pointer_cast<General>(fsm->GetOwner());
	m_bt = GetOwner()->GetComponent<GameServer::Contents::BehaviorTree>();
	m_gameWorld = GetOwner()->GetGameWorld();
}

GameServer::Contents::GeneralRoamingState::GeneralRoamingState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_ROAMING, fsm}, m_accDTForStaminaRecovery{}
{
	auto rootSelector = std::make_unique<GameServer::Contents::SelectorNode>();
	rootSelector->SetTree(GetFSM()->GetOwner()->GetComponent<GameServer::Contents::BehaviorTree>());
	{
		// 점령지를 찾고, 점령지를 향해 달려가는 시퀀스 노드
		auto findOZAndMoveSeq = std::make_unique<GameServer::Contents::SequenceNode>();
		findOZAndMoveSeq->AddChild(std::make_unique<GameServer::Contents::FindOZ>()); 			//	- 1. Action -  점령지를 찾는다.
		findOZAndMoveSeq->AddChild(std::make_unique<GameServer::Contents::MoveToOZ>()); 		//	- 2. Action - 점령지를 찾았으면 점령지를 향해 달려간다

		rootSelector->AddChild(std::move(findOZAndMoveSeq));
	}

	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralRoamingState::~GeneralRoamingState()
{
}

void GameServer::Contents::GeneralRoamingState::Enter(const float dt)
{
	auto const fsm{ GetFSM() };

	// 로밍상태 들어올 때 중립태세로 전환
	GetOwner()->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(GetOwner()->GetID(), GetOwner()->GetStanceType())};
	m_gameWorld->Broadcast(std::move(pb));

	std::cout << "General GeneralRoamingState Enter!" << std::endl;
	
	m_bt = GetOwner()->GetComponent<GameServer::Contents::BehaviorTree>();
	
	if(m_bt) {
		m_bt->SetRoot(m_root.get());
	}
}

void GameServer::Contents::GeneralRoamingState::Exit(const float dt)
{
	std::cout << "General GeneralRoamingState Exit!" << std::endl;
	if(m_bt) {
		m_bt->Reset();
	}
}

void GameServer::Contents::GeneralRoamingState::Update(const float dt)
{
	RecoveryStamina(dt);
	FindGeneral(dt);
}

void GameServer::Contents::GeneralRoamingState::RecoveryStamina(const float dt)
{
	const auto& statInfo = m_owner.lock()->GetStat();
	if(statInfo.currentStamina < statInfo.maxStamina) {
		m_accDTForStaminaRecovery += dt;

		if(m_accDTForStaminaRecovery >= 3.f) {
			m_accDTForStaminaRecovery = 0.F;

			m_owner.lock()->IncStamina(GetOwner()->GetGameObjectData()->staminaRecoveryPerSec);
		}
	}
	else {
		m_accDTForStaminaRecovery = 0.f;
	}
}

void GameServer::Contents::GeneralRoamingState::FindGeneral(const float dt)
{
	const auto& groups{ m_gameWorld->GetGameObjectGroups() };

	for(int i = 0; i < groups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
			continue;

		for(const auto& [id, o] : groups[i]) {

			if(false == IsValidObj(o))
				continue;

			if(GetOwner()->GetID() == id)
				continue;

			if(o->GetTeamType() == GetOwner()->GetTeamType()) continue;

			const auto& targetPos{ o->GetPos() };

			if(GetOwner()->IsTargetInRange(o, 2.f * 2.f)) {
				GetOwner()->GetComponent<GameServer::Contents::BehaviorTree>()->GetBlackboard()->SetValue("Target", id);
				GetFSM()->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DUELING, dt, true);
				return;
			}
		}
	}
}

GameServer::Contents::GeneralDuelingState::GeneralDuelingState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_DUELING , fsm}
{
	auto rootSelector = std::make_unique<GameServer::Contents::SelectorNode>();
	rootSelector->SetTree(GetFSM()->GetOwner()->GetComponent<GameServer::Contents::BehaviorTree>());

	// DefenseSeq
	{
		auto defenseSeq{ std::make_unique<GameServer::Contents::SequenceNode>() };
		{
			defenseSeq->AddChild(std::make_unique<GameServer::Contents::IsTargetAttacking>());

			auto defOrParrySel = std::make_unique<GameServer::Contents::SelectorNode>();
			defOrParrySel->AddChild(std::make_unique<GameServer::Contents::DefaultDefense>());
			// defOrParrySel->AddChild(std::make_unique<Server::Contents::Parrying>());

			defenseSeq->AddChild(std::move(defOrParrySel));
		}

		rootSelector->AddChild(std::move(defenseSeq));
	}

	// AttackSel
	{
		auto attackSel{ std::make_unique<GameServer::Contents::SelectorNode>() };
		attackSel->AddChild(std::make_unique<GameServer::Contents::AttackTry>());
		attackSel->AddChild(std::make_unique<GameServer::Contents::CombatMovement>());

		rootSelector->AddChild(std::move(attackSel));
	}

	m_root = std::move(rootSelector);
}

GameServer::Contents::GeneralDuelingState::~GeneralDuelingState()
{
}

void GameServer::Contents::GeneralDuelingState::Enter(const float dt)
{
	std::cout << "General GeneralDuelingState Enter!" << std::endl;

	GetOwner()->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(GetOwner()->GetID(), GetOwner()->GetStanceType())};
	m_gameWorld->Broadcast(std::move(pb));

	if(m_bt) {
		m_bt->SetRoot(m_root.get());
	}

	GetOwner()->GetComponent<NavAgent>()->StopMove();
}

void GameServer::Contents::GeneralDuelingState::Exit(const float dt)
{
	std::cout << "General GeneralDuelingState Exit!" << std::endl;
	if(m_bt) {
		m_bt->Reset();
	}
}

void GameServer::Contents::GeneralDuelingState::Update(const float dt)
{
	const auto bb{ m_bt->GetBlackboard() };

	if(false == bb->HasKey("Target") || -1 == bb->GetValue<uint64>("Target")) {
		GetFSM()->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING, dt, true);
	}
}

GameServer::Contents::GeneralStunState::GeneralStunState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_STUN, fsm }, m_accDTForStunState{}
{
}

GameServer::Contents::GeneralStunState::~GeneralStunState()
{
}

void GameServer::Contents::GeneralStunState::Enter(const float dt)
{
	std::cout << "General GeneralStunState Enter!" << std::endl;
	m_accDTForStunState = 0.f;

	GetOwner()->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(GetOwner()->GetID(), GetOwner()->GetStanceType()) };
	m_gameWorld->Broadcast(std::move(pb));
}

void GameServer::Contents::GeneralStunState::Exit(const float dt)
{
	std::cout << "General GeneralStunState Exit!" << std::endl;
	m_accDTForStunState = 0.f;
}

void GameServer::Contents::GeneralStunState::Update(const float dt)
{
	const auto fsm{ GetFSM() };

	m_accDTForStunState += dt;

	if(m_accDTForStunState >= 2.f) {
		const auto prevStateType{ fsm->GetPrevStateType() };
		fsm->ChangeState(prevStateType, dt, true);
		return;
	}
}


GameServer::Contents::GeneralDeadState::GeneralDeadState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_DEAD, fsm }, m_accDTForRespawn{}
{
}

GameServer::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void GameServer::Contents::GeneralDeadState::Enter(const float dt)
{
	std::cout << "General GeneralDeadState Enter!" << std::endl;
	m_accDTForRespawn = 0.f;
	if(m_bt)
		m_bt->Reset();	
}

void GameServer::Contents::GeneralDeadState::Exit(const float dt)
{
	std::cout << "General GeneralDeadState Exit!" << std::endl;
	m_accDTForRespawn = 0.f;
}

void GameServer::Contents::GeneralDeadState::Update(const float dt)
{
	m_accDTForRespawn += dt;
	auto const data{ GetOwner()->GetGameObjectData() };

	if(m_accDTForRespawn >= data->respawnTimeSec) {
		GetOwner()->OnRespawn();
	}
}

