#include "stdafxClientFramework.h"
#include "IComponent.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "GameObject.h"

GameObject* IComponent::GetGameObject() const
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	if (!scene)
		return nullptr;
	return scene->TryGetGameObject(GetOwner());
}