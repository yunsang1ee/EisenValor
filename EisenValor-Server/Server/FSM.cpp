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
	if(m_curState)
		m_curState->Update(dt);
}

		if(curState != nextState) {
			m_curState->Exit();
			auto next = m_states.find(nextState);
			m_curState = next->second.get();
			m_curState->Enter();
		}
	}
}

std::shared_ptr<Server::Contents::State> Server::Contents::FSM::GetState(const STATE_TYPE type)
{
	auto iter = m_states.find(type);

	if(iter == m_states.end())
		return nullptr;

	return iter->second;
}

void Server::Contents::FSM::SetCurState(const STATE_TYPE type)
{
	m_curState = GetState(type);
	assert(nullptr != m_curState);
	m_curState->Enter();
}

void Server::Contents::FSM::ChangeState(const STATE_TYPE type)
{
	auto nextState = GetState(type);
	if(nullptr != nextState) {
		m_curState->Exit();
		m_curState = nextState;
		m_curState->Enter();
	}
}

