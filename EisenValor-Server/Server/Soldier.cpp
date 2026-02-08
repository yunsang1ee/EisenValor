#include "pch.h"
#include "Soldier.h"

Server::Contents::Soldier::Soldier(const FB_ENUMS::TEAM_TYPE teamType)
	:Creature{teamType, FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER}
{	static uint32 idGen{ 10000 };
	SetID(idGen);
	idGen++;
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
	GameObject::Update(dt);
}

void Server::Contents::Soldier::OnDeath()
{

}

void Server::Contents::Soldier::ReturnToPool()
{
	std::cout << "Soldier Return Pool!" << std::endl;
	ServerEngine::ObjectPool<Soldier>::Push(this);
}
