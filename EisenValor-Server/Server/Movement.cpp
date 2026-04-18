#include "pch.h"
#include "Movement.h"

#include "GameObject.h"

GameServer::Contents::Movement::Movement()
	:m_maxSpeed{}, m_acceleration{}, m_roatationSpeed{}, m_curVelocity{}
{
}

void GameServer::Contents::Movement::Update(const float dt)
{

}