#pragma once

#include <IComponent.h>

class WorldScene;

class WorldLoadingControllerComponent final : public ComponentBase<WorldLoadingControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "WorldLoadingControllerComponent"; }

	void SetWorldScene(WorldScene& worldScene) { m_worldScene = &worldScene; }
	void OnUpdate(float deltaTime);

private:
	enum class Phase
	{
		WaitForOverlayFrame,
		LoadWorld,
		WaitForPendingLoads,
		WaitForRenderedWorldFrame,
		WaitForRenderWarmup,
		WaitForFrameStability,
		RevealWorld,
		Complete
	};

	WorldScene* m_worldScene = nullptr;
	Phase		m_phase = Phase::WaitForOverlayFrame;
	float		m_warmupElapsedSeconds = 0.0f;
	float		m_stabilizationElapsedSeconds = 0.0f;
	uint32_t	m_consecutiveStableFrames = 0u;
};
