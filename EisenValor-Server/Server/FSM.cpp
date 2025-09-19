#include "pch.h"
#include "FSM.h"

#include "State.h"

void Server::Contents::FSM::Init(const uint8 state)
{
	m_curState = m_states.find(state)->second.get();
}

void Server::Contents::FSM::Update(const float dt)
{
	if(m_curState) {
		const uint8 curState = m_curState->GetType();
		const uint8 nextState = m_curState->Update(dt);

		if(curState != nextState) {
			m_curState->Exit(dt);
			auto next = m_states.find(nextState);
			m_curState = next->second.get();
			m_curState->Enter(dt);
		}
	}
}

void Server::Contents::FSM::AddState(std::unique_ptr<State> state)
{
	state->SetFSM(this);
	m_states.try_emplace(state->GetType(), std::move(state));
}