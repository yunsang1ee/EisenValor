#include "stdafxClientFramework.h"
#include "IComponent.h"
#include "SceneGlobal.h"

template <typename Derived>
GameObject* ComponentBase<Derived>::GetGameObject() const
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	if (!scene)
		return nullptr;
	return scene->TryGetGameObject(GetOwner());
}
