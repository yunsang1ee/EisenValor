#pragma once
#include <IComponent.h>
#include "ButtonUIComponent.h" 
#include "ImageUIComponent.h"  
#include <numbers> 
#include <DirectXMath.h> 

// 1. 방향 열거형 정의
enum class EGuardDir { None, Up, Left, Right };

class BattleUIControllerComponent : public ComponentBase<BattleUIControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "BattleUIControllerComponent"; }

	void OnAttach() override;
	void OnUpdate(float deltaTime);
	void OnDetach() override;

private:
	// 수학 상수
	static constexpr float kPI = std::numbers::pi_v<float>;
	static constexpr float kTwoPI = 2.0f * kPI;
	static constexpr float kDegToRad = kPI / 180.0f;
	static constexpr float kRadToDeg = 180.0f / kPI;

	// UI 설정 상수
	static constexpr float kDefaultRadius = 150.0f; // 기본 반지름
	static constexpr float kInnerRadiusRatio = 0.7f; // 자식 UI 배치될 내부 원 비율
	static constexpr float kUISize = 50.0f; // 자식 UI 크기
	static constexpr float kUIHalfSize = kUISize / 2.0f; // 자식 UI 절반 크기

	// 각도 영역 상수 (Degree, kCamelCase 스타일 적용)
	static constexpr float kUpRegionStart = 30.0f;
	static constexpr float kUpRegionEnd = 150.0f;
	static constexpr float kLeftRegionStart = 150.0f;
	static constexpr float kLeftRegionEnd = 250.0f; // 기존 270에서 250으로 조정
	
	// 리셋 데드존 (입력 취소 구역)
	static constexpr float kDeadZoneRegionStart = 250.0f;
	static constexpr float kDeadZoneRegionEnd = 270.0f;

	static constexpr float kRightRegionStart = 270.0f;
	static constexpr float kRightRegionEnd = 30.0f; // 0도를 넘어가는 특별 처리 필요

	// 데드존 상수
	static constexpr float kMouseDeltaIgnoreSq = 1.0f; // 마우스 이동량 제곱 데드존

	// 누적 방식 관련
	float m_accumulatedDeltaX = 0.0f;
	float m_accumulatedDeltaY = 0.0f;
	static constexpr float kAccumulationThresholdSq = 10.0f; // 누적된 이동량 임계값

private:
	// UI 자식 핸들
	HandleOf<ButtonUIComponent> m_upButtonHandle;
	HandleOf<ButtonUIComponent> m_leftButtonHandle;
	HandleOf<ButtonUIComponent> m_rightButtonHandle;

	HandleOf<ImageUIComponent> m_upImageHandle;
	HandleOf<ImageUIComponent> m_leftImageHandle;
	HandleOf<ImageUIComponent> m_rightImageHandle;

	// 중심점 및 반지름
	float m_centerX = 0.0f;
	float m_centerY = 0.0f;
	float m_radius = kDefaultRadius; // 기본 반지름 적용

	EGuardDir m_currentSelectedDir = EGuardDir::None; // 현재 선택된 가드 방향

private:
	// 헬퍼 함수 선언
	void InitializeChildHandlesAndSetupUI();
	void SetChildUIPositions(); // 중심점, 반지름은 멤버 변수 사용
	void ProcessMouseInput();
	EGuardDir CalculateGuardDirection(float deltaX, float deltaY) const;
	void UpdateUISelection(EGuardDir selectedDir);
	void OnGuardDirectionConfirmed(EGuardDir confirmedDir);

	static float NormalizeAngle(float degrees); // 각도 정규화
};