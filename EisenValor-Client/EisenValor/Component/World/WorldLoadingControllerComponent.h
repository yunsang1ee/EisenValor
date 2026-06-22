#pragma once

#include <IComponent.h>

class WorldLoadingControllerComponent final : public ComponentBase<WorldLoadingControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "WorldLoadingControllerComponent"; }

	void OnUpdate(float deltaTime);

private:
	enum class Phase
	{
		WaitForOverlayFrame,
		LoadWorld,
		WaitForPendingLoads,
		WaitForFinalFrame,
		Complete
	};

	Phase m_phase = Phase::WaitForOverlayFrame;
};
