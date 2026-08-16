#pragma once

#include <IComponent.h>

class RoomSceneControllerComponent final : public ComponentBase<RoomSceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "RoomSceneControllerComponent"; }

	void OnUpdate(float deltaTime);

private:
	uint64 m_lastRevision = std::numeric_limits<uint64>::max();
};
