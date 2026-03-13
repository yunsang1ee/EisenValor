#pragma once

#include <IComponent.h>

class RoomSceneControllerComponent final : public ComponentBase<RoomSceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "RoomSceneControllerComponent"; }

	void OnUpdate(float deltaTime);
};
