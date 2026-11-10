#pragma once

#include <Scene.h>
#include "RenderData/RaytracingFrameRenderData.h"
#include "BoxSceneQuery.h"

class MaterialResource;

class PathTracingDemoScene final : public Scene
{
public:
	void		   OnStartImpl() override;
	void		   OnEndImpl() override;
	void		   CycleColorLighting();
	const wchar_t* GetColorLightingLabel() const;
	void		   RegisterCameraBlocker(
				  HandleOf<GameObject> object, const DirectX::XMFLOAT3& localCenter, const DirectX::XMFLOAT3& localHalfExtents
			  );
	DirectX::XMFLOAT3 MoveCamera(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& displacement, float radius);

protected:
	void OnRegisterCustomComponents() override;

private:
	void				 CreateExhibits();
	HandleOf<GameObject> m_doorRoot;
	void				 CreateColorRoom();
	struct CameraBlocker
	{
		HandleOf<GameObject> object;
		DirectX::XMFLOAT3	 localCenter;
		DirectX::XMFLOAT3	 localHalfExtents;
	};
	std::vector<CameraBlocker> m_cameraBlockers;
	std::vector<SceneQueryBox> m_cameraQueryBoxes;
	struct ColorLight
	{
		HandleOf<GameObject>			  object;
		std::shared_ptr<MaterialResource> material;
		uint32_t						  mask;
	};
	std::vector<ColorLight>			  m_colorLights;
	std::shared_ptr<MaterialResource> m_unlitFixture;
	uint32_t						  m_colorPreset = 0;

	RaytracingSettings m_previousSettings;
	bool			   m_hasPreviousSettings = false;
	bool			   m_previousWarmupAllowed = false;
	bool			   m_previousPerformanceTracking = false;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	bool	 m_previousDebugOverride = false;
	bool	 m_previousSecondaryNee = true;
	bool	 m_previousCameraFrozen = false;
	uint32_t m_previousDebugSource = 0;
	uint32_t m_previousDebugView = 0;
#endif
};
