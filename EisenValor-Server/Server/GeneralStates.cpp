#include "pch.h"
#include "GeneralStates.h"

#include "GameObject.h"
#include "General.h"
#include "BehaviorTree.h"
#include "GameWorld.h"
#include "BehaviorNode.h"
#include "GeneralNodes.h"
#include "NavAgent.h"

Server::Contents::GeneralState::GeneralState(const uint8 stateType, FSM* const fsm)
	:State{stateType}
{
	SetFSM(fsm);

	m_owner = static_cast<General*>(fsm->GetOwner());
	m_bt = m_owner->GetComponent<Server::Contents::BehaviorTree>();
	m_gameWorld = m_owner->GetGameWorld();
}

Server::Contents::GeneralRoamingState::GeneralRoamingState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_ROAMING, fsm}, m_accDTForStaminaRecovery{}
{
	auto rootSelector = std::make_unique<Server::Contents::SelectorNode>();
	rootSelector->SetTree(GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>());
	{
		// 점령지를 찾고, 점령지를 향해 달려가는 시퀀스 노드
		auto findOZAndMoveSeq = std::make_unique<Server::Contents::SequenceNode>();
		findOZAndMoveSeq->AddChild(std::make_unique<Server::Contents::FindOZ>()); 			//	- 1. Action -  점령지를 찾는다.
		findOZAndMoveSeq->AddChild(std::make_unique<Server::Contents::MoveToOZ>()); 		//	- 2. Action - 점령지를 찾았으면 점령지를 향해 달려간다

		rootSelector->AddChild(std::move(findOZAndMoveSeq));
	}

	m_root = std::move(rootSelector);
}

Server::Contents::GeneralRoamingState::~GeneralRoamingState()
{
}

void Server::Contents::GeneralRoamingState::Enter(const float dt)
{
	auto const fsm{ GetFSM() };

	// 로밍상태 들어올 때 중립태세로 전환
	m_owner->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(m_owner->GetID(), m_owner->GetStanceType()) };
	m_gameWorld->Broadcast(std::move(pb));

	std::cout << "General GeneralRoamingState Enter!" << std::endl;
	
	m_bt = m_owner->GetComponent<Server::Contents::BehaviorTree>();
	
	if(m_bt) {
		m_bt->SetRoot(m_root.get());
	}
}

void Server::Contents::GeneralRoamingState::Exit(const float dt)
{
	std::cout << "General GeneralRoamingState Exit!" << std::endl;
	if(m_bt) {
		m_bt->Reset();
	}
}

void Server::Contents::GeneralRoamingState::Update(const float dt)
{
	RecoveryStamina(dt);
	FindGeneral(dt);
}

void Server::Contents::GeneralRoamingState::RecoveryStamina(const float dt)
{
	const auto& statInfo = m_owner->GetStat();
	if(statInfo.currentStamina < statInfo.maxStamina) {
		m_accDTForStaminaRecovery += dt;

		if(m_accDTForStaminaRecovery >= 3.f) {
			m_accDTForStaminaRecovery = 0.F;

			m_owner->IncStamina(m_owner->GetGameObjectData()->staminaRecoveryPerSec);
		}
	}
	else {
		m_accDTForStaminaRecovery = 0.f;
	}
}

void Server::Contents::GeneralRoamingState::FindGeneral(const float dt)
{
	const auto& groups{ m_gameWorld->GetGameObjectGroups() };

	for(int i = 0; i < groups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
			continue;

		for(const auto& [id, o] : groups[i]) {

			auto obj{ o.get() };

			if(false == IsValidObj(obj))
				continue;

			if(m_owner->GetID() == id)
				continue;

			if(obj->GetTeamType() == m_owner->GetTeamType()) continue;

			const auto& targetPos{ obj->GetPos() };

			if(m_owner->IsTargetInRange(obj, 2.f * 2.f)) {
				m_owner->GetComponent<Server::Contents::BehaviorTree>()->GetBlackboard()->SetValue("Target", obj->GetID());
				GetFSM()->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DUELING, dt, true);
				return;
			}
		}
	}
}

Server::Contents::GeneralDuelingState::GeneralDuelingState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_DUELING , fsm}
{
	auto rootSelector = std::make_unique<Server::Contents::SelectorNode>();
	rootSelector->SetTree(GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>());

	// DefenseSeq
	{
		auto defenseSeq{ std::make_unique<Server::Contents::SequenceNode>() };
		{
			defenseSeq->AddChild(std::make_unique<Server::Contents::IsTargetAttacking>());

			auto defOrParrySel = std::make_unique<Server::Contents::SelectorNode>();
			defOrParrySel->AddChild(std::make_unique<Server::Contents::DefaultDefense>());
			// defOrParrySel->AddChild(std::make_unique<Server::Contents::Parrying>());

			defenseSeq->AddChild(std::move(defOrParrySel));
		}

		rootSelector->AddChild(std::move(defenseSeq));
	}

	// AttackSel
	{
		auto attackSel{ std::make_unique<Server::Contents::SelectorNode>() };
		attackSel->AddChild(std::make_unique<Server::Contents::AttackTry>());
		attackSel->AddChild(std::make_unique<Server::Contents::CombatMovement>());

		rootSelector->AddChild(std::move(attackSel));
	}

	m_root = std::move(rootSelector);
}

Server::Contents::GeneralDuelingState::~GeneralDuelingState()
{
}

void Server::Contents::GeneralDuelingState::Enter(const float dt)
{
	std::cout << "General GeneralDuelingState Enter!" << std::endl;

	m_owner->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(m_owner->GetID(), m_owner->GetStanceType()) };
	m_gameWorld->Broadcast(std::move(pb));

	if(m_bt) {
		m_bt->SetRoot(m_root.get());
	}

	m_owner->GetComponent<NavAgent>()->StopMove();
}

void Server::Contents::GeneralDuelingState::Exit(const float dt)
{
	std::cout << "General GeneralDuelingState Exit!" << std::endl;
	if(m_bt) {
		m_bt->Reset();
	}
}

void Server::Contents::GeneralDuelingState::Update(const float dt)
{
	const auto bb{ m_bt->GetBlackboard() };

	if(false == bb->HasKey("Target") || -1 == bb->GetValue<uint32>("Target")) {
		GetFSM()->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING, dt, true);
	}
}

Server::Contents::GeneralStunState::GeneralStunState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_STUN, fsm }, m_accDTForStunState{}
{
}

Server::Contents::GeneralStunState::~GeneralStunState()
{
}

void Server::Contents::GeneralStunState::Enter(const float dt)
{
	std::cout << "General GeneralStunState Enter!" << std::endl;
	m_accDTForStunState = 0.f;

	m_owner->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(m_owner->GetID(), m_owner->GetStanceType()) };
	m_gameWorld->Broadcast(std::move(pb));
}

void Server::Contents::GeneralStunState::Exit(const float dt)
{
	std::cout << "General GeneralStunState Exit!" << std::endl;
	m_accDTForStunState = 0.f;
}

void Server::Contents::GeneralStunState::Update(const float dt)
{
	const auto fsm{ GetFSM() };

	m_accDTForStunState += dt;

	if(m_accDTForStunState >= 2.f) {
		const auto prevStateType{ fsm->GetPrevStateType() };
		fsm->ChangeState(prevStateType, dt, true);
		return;
	}
}


Server::Contents::GeneralDeadState::GeneralDeadState(FSM* const fsm)
	:GeneralState{ FB_ENUMS::GENERAL_STATE_TYPE_DEAD, fsm }, m_accDTForRespawn{}
{
}

Server::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void Server::Contents::GeneralDeadState::Enter(const float dt)
{
	std::cout << "General GeneralDeadState Enter!" << std::endl;
	m_accDTForRespawn = 0.f;
	if(m_bt)
		m_bt->Reset();	
}

void Server::Contents::GeneralDeadState::Exit(const float dt)
{
	std::cout << "General GeneralDeadState Exit!" << std::endl;
	m_accDTForRespawn = 0.f;
}

void Server::Contents::GeneralDeadState::Update(const float dt)
{
	m_accDTForRespawn += dt;
	auto const data{ m_owner->GetGameObjectData() };

	if(m_accDTForRespawn >= data->respawnTimeSec) {
		m_owner->OnRespawn();
	}
}

