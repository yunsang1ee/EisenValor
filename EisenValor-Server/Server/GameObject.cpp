#include "pch.h"
#include "GameObject.h"

Server::Contents::GameObject::GameObject(const GAME_OBJECT_TYPE type, const TEAM_TYPE teamType)
	:m_type{type}, m_teamType{teamType}
{
}

Server::Contents::GameObject::~GameObject()
{
	std::cout << std::format("~GameObject! ID = {}", GetID()) << std::endl;
}

void Server::Contents::GameObject::Update(const float dt)
{
	for(const auto& comp : m_components) {
		if(comp)
			comp->Update(dt);
	}
}