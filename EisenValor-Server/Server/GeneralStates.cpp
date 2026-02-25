#include "pch.h"
#include "GeneralStates.h"

#include "GameObject.h"
#include "General.h"
#include "BehaviorTree.h"
#include "GameWorld.h"
#include "BehaviorNode.h"
#include "GeneralNodes.h"
#include "NavAgent.h"

Server::Contents::GeneralRoamingState::GeneralRoamingState(FSM* const fsm)
	:State{ FB_ENUMS::GENERAL_STATE_TYPE_ROAMING }, m_accDTForStaminaRecovery{}
{
	SetFSM(fsm);

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
	auto const owner{ static_cast<General*>(fsm->GetOwner()) };
	auto const world{ owner->GetGameWorld() };

	// 로밍상태 들어올 때 중립태세로 전환
	owner->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(owner->GetID(), owner->GetStanceType()) };
	world->Broadcast(std::move(pb));

	std::cout << "General GeneralRoamingState Enter!" << std::endl;
	
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	if(bt) {
		bt->SetRoot(m_root.get());
	}
}

void Server::Contents::GeneralRoamingState::Exit(const float dt)
{
	std::cout << "General GeneralRoamingState Exit!" << std::endl;
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	if(bt) {
		bt->Reset();
	}
}

void Server::Contents::GeneralRoamingState::Update(const float dt)
{
	auto const fsm{ GetFSM() };
	auto const owner{ static_cast<Creature*>(fsm->GetOwner()) };
	const auto bt{ owner->GetComponent<Server::Contents::BehaviorTree>() };
	const auto& ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };
	const auto& groups{ world->GetGameObjectGroups() };

	const auto& statInfo = owner->GetStat();
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

	for(int i = 0; i < groups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
			continue;

		for(const auto& [id, o] : groups[i]) {

			auto obj{ o.get() };

			if(false == IsValidObj(obj))
				continue;
			
			if(owner->GetID() == id)
				continue;

			if(obj->GetTeamType() == owner->GetTeamType()) continue;

			const auto& targetPos{ obj->GetPos() };

			if(owner->IsTargetInRange(obj, 2.f * 2.f)) {
				owner->GetComponent<Server::Contents::BehaviorTree>()->GetBlackboard()->SetValue("Target", obj->GetID());
				fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DUELING, dt, true);
				return;
			}
		}
	}

}

Server::Contents::GeneralDuelingState::GeneralDuelingState(FSM* const fsm)
	:State{ FB_ENUMS::GENERAL_STATE_TYPE_DUELING }
{
	SetFSM(fsm);

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
	auto const fsm{ GetFSM() };
	auto const owner{ static_cast<General*>(fsm->GetOwner()) };
	auto const world{ owner->GetGameWorld() };
	std::cout << "General GeneralDuelingState Enter!" << std::endl;
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };

	owner->SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT);
	auto pb{ ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(owner->GetID(), owner->GetStanceType()) };
	world->Broadcast(std::move(pb));

	if(bt) {
		bt->SetRoot(m_root.get());
	}

	owner->GetComponent<NavAgent>()->StopMove();
}

void Server::Contents::GeneralDuelingState::Exit(const float dt)
{
	std::cout << "General GeneralDuelingState Exit!" << std::endl;
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	if(bt) {
		bt->Reset();
	}
}

void Server::Contents::GeneralDuelingState::Update(const float dt)
{
	const auto fsm{ GetFSM() };
	const auto owner{ fsm->GetOwner() };
	const auto bt{ owner->GetComponent<Server::Contents::BehaviorTree>() };
	const auto bb{ bt->GetBlackboard() };

	if(false == bb->HasKey("Target") || -1 == bb->GetValue<uint32>("Target")) {
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING,dt, true);
	}
}

Server::Contents::GeneralStunState::GeneralStunState(FSM* const fsm)
	:State{ FB_ENUMS::GENERAL_STATE_TYPE_STUN }, m_accDTForStunState{}
{
	SetFSM(fsm);

}

Server::Contents::GeneralStunState::~GeneralStunState()
{
}

void Server::Contents::GeneralStunState::Enter(const float dt)
{
	std::cout << "General GeneralStunState Enter!" << std::endl;
	m_accDTForStunState = 0.f;
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
	:State{ FB_ENUMS::GENERAL_STATE_TYPE_DEAD }, m_accDTForRespawn{}
{
	SetFSM(fsm);
}

Server::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void Server::Contents::GeneralDeadState::Enter(const float dt)
{
	std::cout << "General GeneralDeadState Enter!" << std::endl;
	m_accDTForRespawn = 0.f;

	const auto fsm{ GetFSM() };
	const auto owner{ fsm->GetOwner() };
	const auto bt{ owner->GetComponent<Server::Contents::BehaviorTree>() };
	const auto bb{ bt->GetBlackboard() };

	if(bt)
		bt->Reset();	
}

void Server::Contents::GeneralDeadState::Exit(const float dt)
{
	std::cout << "General GeneralDeadState Exit!" << std::endl;
	m_accDTForRespawn = 0.f;
}

void Server::Contents::GeneralDeadState::Update(const float dt)
{
	m_accDTForRespawn += dt;
	auto const fsm{ GetFSM() };
	auto const owner{ fsm->GetOwner() };
	auto const data{ owner->GetGameObjectData() };

	if(m_accDTForRespawn >= data->respawnTimeSec) {
		static_cast<General*>(owner)->OnRespawn();
	}
}