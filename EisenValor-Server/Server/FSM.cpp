#include "pch.h"
#include "FSM.h"

#include "State.h"

void Server::Contents::FSM::Update(const float dt)
{
	if(m_curState)
		m_curState->Update(dt);
}

void Server::Contents::FSM::AddState(std::shared_ptr<State> state)
{
	const STATE_TYPE type = state->GetType();
	if(nullptr == GetState(type)) {
		m_states.insert(std::make_pair(type,state));
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

