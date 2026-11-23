#pragma once
#include <Scene.h>
class WorldScene final : public Scene
{
protected:
	void OnRegisterCustomComponents();
	void OnRegisterCustomSceneComponentDecoders() override;

public:
	void OnStartImpl() override;
	void CreateSceneObjects();
	void OnEndImpl() override;

private:
#if defined(ENABLE_INGAME_PERFORMANCE_OVERLAY)
	bool m_previousPerformanceTracking = false;
#endif
	void CreateDebugNavMeshPreview();
	void CreateDebugMapObjPreview();
};
