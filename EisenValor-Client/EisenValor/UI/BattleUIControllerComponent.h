#pragma once
#include <IComponent.h>
#include "ButtonUIComponent.h" 
#include "ImageUIComponent.h"  
#include <numbers> 
#include <DirectXMath.h> 

// TODO: 없애기

// 1. 방향 열거형 정의
enum class EGuardDir { None, Up, Left, Right };

// 2. 공격 타입 열거형 정의
enum class EAttackType { None, Light, Strong };

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
	static constexpr float kDefaultRadius = 80.0f; // 기본 반지름
	static constexpr float kInnerRadiusRatio = 0.7f; // 자식 UI 배치될 내부 원 비율
	static constexpr float kUISize = 30.0f; // 자식 UI 크기
	static constexpr float kUIHalfSize = kUISize / 2.0f;

	// 거리 비례 스케일링 상수
	static constexpr float kReferenceDistance = 10.0f; // 기준 거리 (이 거리에서 스케일 1.0)
	static constexpr float kMinScale = 0.5f;		   // 최소 스케일
	static constexpr float kMaxScale = 2.0f;		   // 최대 스케일

	// 각도 영역 상수 (Degree, kCamelCase 스타일 적용)
	static constexpr float kUpRegionStart = 30.0f;
	static constexpr float kUpRegionEnd = 150.0f;
	static constexpr float kLeftRegionStart = 150.0f;
	static constexpr float kLeftRegionEnd = 250.0f;
	
	// 리셋 데드존 (입력 취소 구역)
	static constexpr float kDeadZoneRegionStart = 250.0f;
	static constexpr float kDeadZoneRegionEnd = 270.0f;

	static constexpr float kRightRegionStart = 270.0f;
	static constexpr float kRightRegionEnd = 30.0f;

	// 데드존 상수
	static constexpr float kMouseDeltaIgnoreSq = 1.0f; // 너무 작은 값은 무시 

	// 누적 방식 관련
	float m_accumulatedDeltaX = 0.0f;
	float m_accumulatedDeltaY = 0.0f;
	static constexpr float kAccumulationThresholdSq = 20.0f; // 누적된 이동량 임계값

private:
	// UI 자식 핸들
	HandleOf<ButtonUIComponent> m_upButtonHandle;
	HandleOf<ButtonUIComponent> m_leftButtonHandle;
	HandleOf<ButtonUIComponent> m_rightButtonHandle;

	// 일반/약공격용 이미지
	HandleOf<ImageUIComponent> m_upImageHandle;
	HandleOf<ImageUIComponent> m_leftImageHandle;
	HandleOf<ImageUIComponent> m_rightImageHandle;

	// 강공격용 이미지
	HandleOf<ImageUIComponent> m_upStrongImageHandle;
	HandleOf<ImageUIComponent> m_leftStrongImageHandle;
	HandleOf<ImageUIComponent> m_rightStrongImageHandle;

	// 추적 대상 트랜스폼
	HandleOf<Transform> m_targetTrHandle; 

	// 중심점 및 반지름
	float m_centerX = 0.0f;
	float m_centerY = 0.0f;
	float m_radius = kDefaultRadius; // 기본 반지름 적용

	EGuardDir m_currentSelectedDir = EGuardDir::None; // 현재 선택된 가드 방향
	EAttackType m_currentAttackType = EAttackType::None; // 현재 시도 중인 공격 타입

	// 캐싱된 텍스처 ID
	uint32_t m_normalTexId = 0;
	uint32_t m_hoverTexId = 0;
	uint32_t m_lightAttackTexId = 0;
	uint32_t m_strongAttackTexId = 0;

private:
	// 헬퍼 함수 선언
	void InitializeChildHandlesAndSetupUI();
	void SetChildUIPositions(float scale = 1.0f); // 스케일 인자 추가
	void ProcessMouseInput();
	EGuardDir CalculateGuardDirection(float deltaX, float deltaY) const;
	void UpdateUISelection(EGuardDir selectedDir, EAttackType attackType);
	void OnGuardDirectionConfirmed(EGuardDir confirmedDir, EAttackType attackType);

	void		 UpdatePositionFromTarget();	// 타겟 위치에 따라 중심점 갱신 (World To Screen)

	static float NormalizeAngle(float degrees); // 각도 정규화

public:
	void SetTarget(HandleOf<Transform> targetHandle) { m_targetTrHandle = targetHandle; }
};