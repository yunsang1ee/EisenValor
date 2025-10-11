#include "pch.h"
#include "FSM.h"

#include "State.h"
	
void Server::Contents::FSM::InitStartState(const uint8 state)
{
	auto iter = m_states.find(state);
	if(iter != m_states.end()) {
		m_curState = iter->second.get();
		m_curState->Enter();
	}
}

void Server::Contents::FSM::Update(const float dt)
{
	if(m_curState)
		m_curState->Update(dt);
}

void Server::Contents::FSM::AddState(std::unique_ptr<State> state)
{
	state->SetFSM(this);
	if(m_states.find(state->GetStateType()) == m_states.end())
		m_states.try_emplace(state->GetStateType(), std::move(state));
}

void Server::Contents::FSM::ChangeState(const uint8 nextState)
{
	m_curState->Exit();
	auto iter = m_states.find(nextState);
	if(iter != m_states.end()) {
		m_curState = iter->second.get();
		m_curState->Enter();
	}
}
