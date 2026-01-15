#include "stdafxClientFramework.h"
#include "IComponent.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "GameObject.h"

GameObject* IComponent::GetGameObject() const
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
		return nullptr;
	return scene->TryGetGameObject(GetOwner());
}

void IComponent::SetEnabled(bool enabled)
{
	if (m_enabled == enabled)
	{
		return;
	}

	m_enabled = enabled;
	enabled ? OnEnable() : OnDisable();
}