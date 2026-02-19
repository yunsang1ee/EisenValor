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
	:State(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING)
{
	SetFSM(fsm);

	auto rootSelector = std::make_unique<Server::Contents::SelectorNode>();
	rootSelector->SetTree(GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>());
	{
		// 점령지를 찾고, 점령지를 향해 달려가는 시퀀스 노드
		auto seqFindOZAndMove = std::make_unique<Server::Contents::SequenceNode>();
		seqFindOZAndMove->AddChild(std::make_unique<Server::Contents::ActionFindOZ>()); 		//	- 1. Action -  점령지를 찾는다.
		seqFindOZAndMove->AddChild(std::make_unique<Server::Contents::ActionMoveToOZ>()); 		//	- 2. Action - 점령지를 찾았으면 점령지를 향해 달려간다

		rootSelector->AddChild(std::move(seqFindOZAndMove));
	}

	m_root = std::move(rootSelector);
}

Server::Contents::GeneralRoamingState::~GeneralRoamingState()
{
}

void Server::Contents::GeneralRoamingState::Enter(const float dt)
{
	std::cout << "General GeneralRoamingState Enter!" << std::endl;
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	if(bt) {
		bt->SetRoot(m_root.get());
		bt->Reset();
	}
}

void Server::Contents::GeneralRoamingState::Exit(const float dt)
{
	std::cout << "General GeneralRoamingState Exit!" << std::endl;
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	if(bt) {
		bt->SetRoot(m_root.get());
		bt->Reset();
	}
}

void Server::Contents::GeneralRoamingState::Update(const float dt)
{
	auto const fsm{ GetFSM() };
	auto const owner{ fsm->GetOwner() };
	const auto& ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };
	const auto& groups{ world->GetGameObjectGroups() };

	for(int i = 0; i < groups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
			continue;

		for(const auto& [id, o] : groups[i]) {
			
			if(owner->GetID() == id) continue;

			const auto& targetPos{ o->GetPos() };

			const float distToTargetSq{ (targetPos - ownerPos).LengthSquared() };

			if(distToTargetSq <= 2.f * 2.f) {
				std::cout << "Stop!" << std::endl;
				owner->GetComponent<Server::Contents::BehaviorTree>()->GetBlackboard()->SetValue("Target", o->GetID());
				fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DUELING, dt, true);
				return;
			}

		}
	}

	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	
	if(bt)
		bt->Update(dt);
}
	
Server::Contents::GeneralDuelingState::GeneralDuelingState(FSM* const fsm)
	:State(FB_ENUMS::GENERAL_STATE_TYPE_DUELING)
{
	SetFSM(fsm);

	// TODO: 행동트리 구현
	auto rootSelector = std::make_unique<Server::Contents::SelectorNode>();
	rootSelector->SetTree(GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>());

	// seqDefense
	{
		auto seqDefense{ std::make_unique<Server::Contents::SequenceNode>() };
		{
			seqDefense->AddChild(std::make_unique<Server::Contents::ConditionIsTargetAttacking>());
			seqDefense->AddChild(std::make_unique<Server::Contents::ActionDefense>());
		}

		rootSelector->AddChild(std::move(seqDefense));
	}

	// seqAtk
	{
		auto seqAtk{ std::make_unique<Server::Contents::SequenceNode>() };
		rootSelector->AddChild(std::move(seqAtk));
	}
}

Server::Contents::GeneralDuelingState::~GeneralDuelingState()
{
}

void Server::Contents::GeneralDuelingState::Enter(const float dt)
{
	auto const fsm{ GetFSM() };
	auto const owner{ fsm->GetOwner() };
	std::cout << "General GeneralDuelingState Enter!" << std::endl;
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	if(bt) {
		bt->SetRoot(m_root.get());
		bt->Reset();
	}

	owner->GetComponent<NavAgent>()->StopMove();
}

void Server::Contents::GeneralDuelingState::Exit(const float dt)
{
	std::cout << "General GeneralDuelingState Exit!" << std::endl;
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };
	if(bt) {
		bt->SetRoot(m_root.get());
		bt->Reset();
	}
}

void Server::Contents::GeneralDuelingState::Update(const float dt)
{
	auto const bt{ GetFSM()->GetOwner()->GetComponent<Server::Contents::BehaviorTree>() };

	if(bt)
		bt->Update(dt);
/*
	- 상대가 공격 O
		- 약공격 방어 확률 30%
		- 강공격 방어 확률 90%
		- 반격 확률 20%
	- 상대가 공격 X
		- 공격할 확률 60% (이때 스탠스를 바꿀 확률 50%)
		- 약공격 확률 40%
			- 약공격으로 시작하는 콤보 중 하나 실행확률 30%
		- 강공격 확률 50%
			- 페이크확률 60%
			- 강공격으로 시작하는 콤보 중 하나 실행확률 40%
		- 방어해제공격 10%
	- 움직이거나 스탠스만 변경할 확률 40%
*/
}

Server::Contents::GeneralDeadState::GeneralDeadState(FSM* const fsm)
	:State(FB_ENUMS::GENERAL_STATE_TYPE_DEAD), m_accDTForRespawn{}
{
	SetFSM(fsm);
}

Server::Contents::GeneralDeadState::~GeneralDeadState()
{
}

void Server::Contents::GeneralDeadState::Enter(const float dt)
{
	std::cout << "General GeneralDeadState Enter!" << std::endl;
	// TODO: 행동트리 생성
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
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING, dt, true);
	}
}
