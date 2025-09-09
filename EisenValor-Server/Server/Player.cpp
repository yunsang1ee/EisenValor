#include "pch.h"
#include "Player.h"

#include "NPC.h"
#include "GameRoom.h"

Server::Contents::Player::Player()
	:Creature(GAME_OBJECT_TYPE::PLAYER, TEAM_TYPE::ALLY)
{
	std::cout << std::format("Player! ID = {}", GetID()) << std::endl;
}

Server::Contents::Player::~Player()
{
	std::cout << std::format("~Player! ID = {}", GetID()) << std::endl;
}

void Server::Contents::Player::AddSoldier(std::shared_ptr<Server::Contents::NPC> soldier, const Vec3& localOffset)
{
	const std::lock_guard<std::shared_mutex> lk(m_soldierlk);
	m_soldiers.emplace_back(SoldierFormationData{ soldier, localOffset });
}

const std::vector<Server::Contents::SoldierFormationData>& Server::Contents::Player::GetNpcs() noexcept
{
	const std::shared_lock<std::shared_mutex> lk( m_soldierlk );
	return m_soldiers;
}

void Server::Contents::Player::Update(const float dt)
{

}