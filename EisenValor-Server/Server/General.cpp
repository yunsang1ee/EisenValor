#include "pch.h"
#include "General.h"

#include "Soldier.h"

Server::Contents::General::General()
{
}

void Server::Contents::General::AddSoldier(std::shared_ptr<Server::Contents::Soldier> soldier)
{
	m_soldiers.push_back(soldier);
}