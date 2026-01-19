#pragma once
#include <IComponent.h>
#include "UIGlobal.h"
#include "BattleUIComponent.h"
#include <DxSwapChain.h>
#include <DxRendererGlobal.h>
#include <InputGlobal.h>
#include <numbers>

class BattleUI : public ComponentBase<BattleUI>
{
private:
	// 수학 상수
	static constexpr float PI = std::numbers::pi_v<float>;
	static constexpr float TWO_PI = 2.0f * PI;
	static constexpr float DEG_TO_RAD = PI / 180.0f;
	static constexpr float RAD_TO_DEG = 180.0f / PI;

	// UI 설정 상수
	static constexpr float DEFAULT_RADIUS = 150.0f;
	static constexpr float INNER_RADIUS_RATIO = 0.7f;
	static constexpr float UI_SIZE = 50.0f;
	static constexpr float UI_HALF_SIZE = UI_SIZE / 2.0f;

	// 기하학적 상수
	static constexpr float COS_30_DEG = 0.866025403784f; // √3/2
	static constexpr float SIN_30_DEG = 0.5f;

	// 각도 영역 상수
	static constexpr float UP_REGION_START = 30.0f;
	static constexpr float UP_REGION_END = 150.0f;
	static constexpr float LEFT_REGION_START = 150.0f;
	static constexpr float LEFT_REGION_END = 270.0f;
	static constexpr float RIGHT_REGION_START = 270.0f;
	static constexpr float RIGHT_REGION_END = 30.0f;

#ifdef _DEBUG
	// 디버그 렌더링 상수
	static constexpr int   DEBUG_CIRCLE_POINTS = 32;
	static constexpr float DEBUG_DOT_SIZE = 5.0f;
	static constexpr float DEBUG_DOT_HALF_SIZE = DEBUG_DOT_SIZE / 2.0f;
	static constexpr float DEBUG_LINE_SIZE = 3.0f;
	static constexpr float DEBUG_LINE_HALF_SIZE = DEBUG_LINE_SIZE / 2.0f;
	static constexpr int   DEBUG_LINE_SEGMENTS = 20;
#endif

private:
	float m_centerX = 0.0f;
	float m_centerY = 0.0f;
	float m_radius = DEFAULT_RADIUS;

	static float NormalizeAngle(float degrees)
	{
		while (degrees < 0.0f)
			degrees += 360.0f;
		while (degrees >= 360.0f)
			degrees -= 360.0f;
		return degrees;
	}

public:
	static constexpr const char* GetStaticTypeName() { return "BattleUISystem"; }

	// IComponent 라이프사이클
	void OnAttach() override;
	void OnUpdate(float deltaTime);
	void OnDetach() override;

private:
	using UIHandle = UIGlobal::UIHandle;

	UIHandle m_upUIHandle;
	UIHandle m_leftUIHandle;
	UIHandle m_rightUIHandle;

	std::vector<UIHandle> m_debugCircleHandles;
	std::vector<UIHandle> m_debugRegionHandles;

public:
	// 기존 함수들
	void  InitializeBattleUI();
	void  SetCenterPosition(float x, float y);
	void  SetCenterPositionToScreenCenter();
	void  SetRadius(float radius) { m_radius = radius; }
	float GetRadius() const { return m_radius; }

#ifdef _DEBUG
	void RenderDebugCircle();
	void RenderDebugRegions();
	void ClearDebugCircle();
#endif

private:
	void ProcessMouseInput();
	int	 GetRegionIndex(float angleDegrees) const;
	void UpdateUISelection(int selectedRegion);
	void OnRegionClicked(int regionIndex);
};