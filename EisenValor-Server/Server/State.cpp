#include "pch.h"
#include "State.h"

GameServer::Contents::State::State(const uint8 type)
	:m_type{ type }, m_fsm{ nullptr }
{
}