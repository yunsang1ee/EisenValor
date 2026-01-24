#include "pch.h"
#include "Soldier.h"

Server::Contents::Soldier::Soldier(const FB_ENUMS::TEAM_TYPE teamType)
	:Creature{teamType, FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER}
{
}

Server::Contents::Soldier::~Soldier()
{
}

void Server::Contents::Soldier::OnCollisionEnter(Collider* const other)
{

}

void Server::Contents::Soldier::OnCollisionStay(Collider* const other)
{

}

void Server::Contents::Soldier::OnCollisionExit(Collider* const other)
{

}

void Server::Contents::Soldier::Update(const float dt)
{
	// TODO: 이동 시 이동패킷 보내주기
}

void Server::Contents::Soldier::OnDeath()
{

}