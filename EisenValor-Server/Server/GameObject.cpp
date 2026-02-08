#include "pch.h"
#include "GameObject.h"

Server::Contents::GameObject::GameObject(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type)
	:m_type{ type }, m_teamType{ teamType }, m_scale{1.f}, m_isCreature{false}, m_active{true}
{
}

Server::Contents::GameObject::~GameObject()
{
	std::cout << std::format("~GameObject! ID = {}", GetID()) << std::endl;
}

Vec3 Server::Contents::GameObject::GetForwardDir()
{
	const float yawRad{ Deg2Rad(m_posInfo.rot.y)};
	Vec3 forward;
	forward.x = sinf(yawRad);
	forward.y = 0.f;
	forward.z = cosf(yawRad);
	forward.Normalize();

	return forward;
}

void Server::Contents::GameObject::Update(const float dt)
{
	for(const auto& comp : m_components) {
		if(comp)
			comp->Update(dt);
	}

	for(const auto& script : m_scripts)
		if(script)
			script->Update(dt);
}