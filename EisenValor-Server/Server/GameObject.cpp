#include "pch.h"
#include "GameObject.h"

Server::Contents::GameObject::GameObject(const FB_ENUMS::GAME_OBJECT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType)
	:m_type{type}, m_teamType{teamType}
{
}

Server::Contents::GameObject::~GameObject()
{
	std::cout << std::format("~GameObject! ID = {}", GetID()) << std::endl;
}

const Vec3 Server::Contents::GameObject::GetForwardDir()
{
	Vec3 forward;
	forward.x = sinf(m_kinematicInfo.rotation.y);
	forward.y = 0.f;             // 수평면만 고려
	forward.z = cosf(m_kinematicInfo.rotation.y);
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