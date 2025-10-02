#include "pch.h"
#include "CommonStates.h"

Server::Contents::IdleState::IdleState(const uint8 type)
	:State{type}
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
