#include "pch.h"
#include "GameObject.h"

#include "Creature.h"

GameServer::Contents::GameObject::GameObject(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type)
	:m_objType{ type }, m_teamType{ teamType }, m_scale{1.f}, m_isCreature{false}, m_active{true}, m_look{}, m_rotateSpeed{1.f}
{
}

GameServer::Contents::GameObject::~GameObject()
{
	std::cout << std::format("~GameObject! ID = {}", GetID()) << std::endl;
}

GameServer::Contents::Script* GameServer::Contents::GameObject::GetScript(const std::string_view name)
{
	const auto iter = std::find_if(m_scripts.begin(), m_scripts.end(), [name](const auto& script) {
		return script->GetName() == name.data();
		});

	if(iter == m_scripts.end()) {
		return nullptr;
	}

	return iter->get();
}

Vec3 GameServer::Contents::GameObject::GetForwardDir()
{
	const float yawRad{ Deg2Rad(m_posInfo.rot.y)};
	Vec3 forward;
	forward.x = sinf(yawRad);
	forward.y = 0.f;
	forward.z = cosf(yawRad);
	forward.Normalize();

	return forward;
}

bool GameServer::Contents::GameObject::IsTargetInRange(std::shared_ptr<GameObject> const target, const float rangeSq)
{
	if(nullptr == target)
		return false;

	if(false == target->IsActive())
		return false;

	const auto& myPos{ GetPos() };
	const auto& targetPos{ target->GetPos() };

	const float distToTargetSq{ GetDistSq(myPos, targetPos) };
	
	if(distToTargetSq <= rangeSq)
		return true;
	
	return false;
}

bool GameServer::Contents::GameObject::IsSameTeam(std::shared_ptr<GameObject> const other)
{
	const auto otherTeamType{ other->GetTeamType() };

	if(otherTeamType == m_teamType)
		return true;

	return false;
}

void GameServer::Contents::GameObject::Update(const float dt)
{
	LookAt(m_look, dt);

	for(const auto& comp : m_components) {
		if(comp)
			comp->Update(dt);
	}

	for(const auto& script : m_scripts)
		if(script)
			script->Update(dt);
}

void GameServer::Contents::GameObject::LookAt(const Vec3& lookAt, const float dt)
{
	Vec3 dir{ lookAt - m_posInfo.pos };
	dir.y = 0.0f;

	if(dir.LengthSquared() < 0.0001f) return;

	const float targetYaw{ Rad2Deg(atan2f(dir.x, dir.z))};
	const float currentYaw{ m_posInfo.rot.y };

	const float deltaYaw{ NormalizeAngle(targetYaw - currentYaw) };
	m_posInfo.rot.y = NormalizeAngle(currentYaw + (deltaYaw * dt * m_rotateSpeed));
}