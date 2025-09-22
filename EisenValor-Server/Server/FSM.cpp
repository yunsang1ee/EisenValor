#include "pch.h"
#include "FSM.h"

#include "State.h"

void Server::Contents::FSM::InitStartState(const uint8 state)
{
	m_curState = m_states.find(state)->second.get();
	m_curState->Enter();
}

void Server::Contents::FSM::Update(const float dt)
{
	if(m_curState) {
		const auto curState = m_curState->GetStateType();
		const auto nextState = m_curState->Update(dt);

		if(curState != nextState) {
			m_curState->Exit();
			m_curState = m_states.find(nextState)->second.get();
			m_curState->Enter();
		}
	}
}

void Server::Contents::FSM::AddState(std::unique_ptr<State> state)
{
	state->SetFSM(this);
	m_states.try_emplace(state->GetStateType(), std::move(state));
}