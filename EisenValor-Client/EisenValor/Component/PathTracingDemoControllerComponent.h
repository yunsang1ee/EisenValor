#pragma once

#include <IComponent.h>
#include "GameObject.h"
#include "TextUIComponent.h"
#include "Util/PerformanceStatsText.h"
#include <functional>

class PathTracingDemoDoorComponent;

class PathTracingDemoControllerComponent final : public ComponentBase<PathTracingDemoControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "PathTracingDemoControllerComponent"; }
	void						 OnStart() override;
	void						 OnUpdate(float deltaTime);
	void						 SetDoorObject(HandleOf<GameObject> door) { m_doorObject = door; }

private:
	void						  SetView(uint32_t index);
	void						  CycleRenderer();
	void						  CreateHUD();
	void						  RefreshLabels();
	void						  UpdatePerformanceStats(float deltaTime);
	void						  UpdateButtonAvailability();
	PathTracingDemoDoorComponent* GetDoor() const;
	void						  ToggleDoorAnimation();
	void						  ToggleDoorOpen();
	void						  CycleColorLighting();
	void						  ReserveWidget(
								 const char*				name,
								 Vec2						anchor,
								 Vec2						minimum,
								 Vec2						maximum,
								 const wchar_t*				label,
								 float						fontSize,
								 std::function<void()>		action = {},
								 HandleOf<TextUIComponent>* textHandle = nullptr
							 );

	HandleOf<GameObject>			  m_hudRoot;
	HandleOf<GameObject>			  m_doorObject;
	HandleOf<TextUIComponent>		  m_doorText;
	HandleOf<TextUIComponent>		  m_colorLightText;
	std::vector<HandleOf<GameObject>> m_buttonObjects;
	HandleOf<TextUIComponent>		  m_viewText;
	HandleOf<TextUIComponent>		  m_rendererText;
	HandleOf<TextUIComponent>		  m_descriptionText;
	HandleOf<TextUIComponent>		  m_statusText;
	HandleOf<TextUIComponent>		  m_performanceText;
	PerformanceStatsText			  m_performanceStats;
	uint32_t						  m_viewIndex = 0;
	uint32_t						  m_readyFrames = 0;
	bool							  m_hudVisible = true;
	bool							  m_wasLooking = false;
	float							  m_pitch = 0.0f;
	float							  m_yaw = 0.0f;
	float							  m_labelElapsed = 0.0f;
};
