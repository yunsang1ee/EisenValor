#include "pch.h"
#include "State.h"

Server::Contents::State::State(const uint8 type)
	:m_type(type)
{
}

Server::Contents::State::~State()
{
}

Server::Contents::IdleState::IdleState(const uint8 type)
	:State{ type }
{
}

Server::Contents::RunState::RunState(const uint8 type)
	:State{ type }
{
}

Server::Contents::AttackState::AttackState(const uint8 type)
	:State{ type }
{
}

Server::Contents::DefenseState::DefenseState(const uint8 type)
	:State{ type }
{
}

Server::Contents::DeadState::DeadState(const uint8 type)
	:State{ type }
{
}
