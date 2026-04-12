#include "pch.h"
#include "GameObject.h"

#include "Creature.h"

GameServer::Contents::GameObject::GameObject(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type)
	:m_objType{ type }, m_teamType{ teamType }, m_scale{1.f}, m_isCreature{false}, m_active{true}
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

bool GameServer::Contents::GameObject::IsTargetInRange(std::shared_ptr<GameObject> const target, const float rangeSq)
{
	if(nullptr == target)
		return false;

	if(false == target->IsActive())
		return false;

	const auto& myPos{ GetPosition() };
	const auto& targetPos{ target->GetPosition() };

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
	m_transform.UpdateRotation(dt);

	for(const auto& comp : m_components) {
		if(comp)
			comp->Update(dt);
	}

	for(const auto& script : m_scripts)
		if(script)
			script->Update(dt);

	OnPostComponentUpdate(dt);

	m_transform.CommitPosition();
}