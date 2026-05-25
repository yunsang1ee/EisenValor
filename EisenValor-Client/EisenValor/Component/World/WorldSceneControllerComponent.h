#pragma once

#include <IComponent.h>


class WorldSceneControllerComponent final : public ComponentBase<WorldSceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "WorldSceneControllerComponent"; }

	void OnUpdate(float deltaTime);
};