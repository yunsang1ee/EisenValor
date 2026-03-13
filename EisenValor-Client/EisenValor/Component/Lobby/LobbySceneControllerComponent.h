#pragma once

#include <IComponent.h>

class LobbySceneControllerComponent final : public ComponentBase<LobbySceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "LobbySceneControllerComponent"; }

	void OnUpdate(float deltaTime);
};
