#pragma once

#include <IComponent.h>

class LobbySceneControllerComponent final : public ComponentBase<LobbySceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "LobbySceneControllerComponent"; }

	void OnUpdate(float deltaTime);

private:
	uint64 m_lastRevision = std::numeric_limits<uint64>::max();
};
