#pragma once
#include <IComponent.h>
#include "ButtonUIComponent.h" 
#include "ImageUIComponent.h"  
#include <numbers> 
#include <DirectXMath.h> 
#include <optional>
#include <functional>
#include "Packets/Enums_generated.h"

using namespace FB_ENUMS;

class TextureResource;

class BattleUIControllerComponent : public ComponentBase<BattleUIControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "BattleUIControllerComponent"; }

	enum class ControlType
	{
		Local,
		Remote
	};

	// Observer Pattern Listener
	struct StanceChangeListener {
		HandleOf<GameObject> observerHandle;
		std::function<void(GENERAL_ATTACK_DIR_TYPE, std::optional<GENERAL_ATTACK_TYPE>)> callback;
	};

	void OnAttach() override;
	void OnStart() override;
	void OnUpdate(float deltaTime);
	void OnDestroy() override;

	void SetControlMode(ControlType mode) { m_controlMode = mode; }
	void InitStance(GENERAL_STANCE_TYPE stance); 
	void TriggerAttackRemote(GENERAL_ATTACK_TYPE type, GENERAL_ATTACK_DIR_TYPE dir); // 원격 공격 피드백
	void UpdateUISelection(GENERAL_ATTACK_DIR_TYPE selectedDir, std::optional<GENERAL_ATTACK_TYPE> attackType);
	void ToggleUI(bool isActive); 
	
	// FSM에서 쓸 현재 공격 모드 가져오는 함수
	std::optional<GENERAL_ATTACK_TYPE> GetCurrentAttackType() const { return m_currentAttackType; }
	GENERAL_STANCE_TYPE				   GetStance() const;

	void AddListener(HandleOf<GameObject> observerHandle, std::function<void(GENERAL_ATTACK_DIR_TYPE, std::optional<GENERAL_ATTACK_TYPE>)> callback) {
		m_listeners.push_back({observerHandle, callback});
	}

private:
	// 수학 상수
	static constexpr float kPI = std::numbers::pi_v<float>;
	static constexpr float kTwoPI = 2.0f * kPI;
	static constexpr float kDegToRad = kPI / 180.0f;
	static constexpr float kRadToDeg = 180.0f / kPI;

	// UI 설정 상수
	static constexpr float kcontainerHalfSize = 38.0f;
	static constexpr float kDefaultRadius = 3.0f; // 기본 반지름
	static constexpr float kInnerRadiusRatio = 1.0f; // 자식 UI 배치될 내부 원 비율
	static constexpr float kUISize = 50.0f; // 자식 UI 크기
	static constexpr float kUIHalfSize = kUISize / 2.0f;

	// 거리 비례 스케일링 상수
	static constexpr float kReferenceDistance = 10.0f; // 기준 거리 (이 거리에서 스케일 1.0)
	static constexpr float kMinScale = 0.5f;		   // 최소 스케일
	static constexpr float kMaxScale = 5.0f;		   // 최대 스케일

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
	static constexpr float kMouseDeltaIgnoreSq = 10.0f; // 너무 작은 값은 무시 

	// 누적 방식 관련
	float m_accumulatedDeltaX = 0.0f;
	float m_accumulatedDeltaY = 0.0f;
	static constexpr float kAccumulationThresholdSq = 3000.0f; // 누적된 이동량 임계값

	// 입력 버퍼링 (범위공격 동시 입력 판정용)
	bool  m_pendingLeftClick = false;
	bool  m_pendingRightClick = false;
	float m_inputBufferTimer = 0.0f;
	static constexpr float kInputBufferDuration = 10.0f / 60.0f; // 10프레임

	ControlType m_controlMode = ControlType::Local;

private:
	// UI 루트 및 자식 핸들
	HandleOf<GameObject> m_uiRootObjHandle;
	HandleOf<ButtonUIComponent> m_upButtonHandle;
	HandleOf<ButtonUIComponent> m_leftButtonHandle;
	HandleOf<ButtonUIComponent> m_rightButtonHandle;

	// 일반/약공격용 이미지
	HandleOf<ImageUIComponent> m_upImageHandle;
	HandleOf<ImageUIComponent> m_leftImageHandle;
	HandleOf<ImageUIComponent> m_rightImageHandle;


	// 중심점 및 반지름
	float m_centerX = 0.0f;
	float m_centerY = 0.0f;
	float m_radius = kDefaultRadius; // 기본 반지름 적용

	GENERAL_ATTACK_DIR_TYPE			   m_currentSelectedDir = GENERAL_ATTACK_DIR_TYPE_NONE;  // 현재 선택된 가드 방향
	std::optional<GENERAL_ATTACK_TYPE> m_currentAttackType = std::nullopt;					 // 현재 시도 중인 공격 타입 (없으면 nullopt)
	bool							   m_isAttackValid = true;								 // 현재 입력(클릭)이 유효한지 여부 (방향 변경 시 false)

	// 공격 피드백용 타이머 (초 단위)
	float               m_attackFeedbackTimer = 0.0f;
	GENERAL_ATTACK_TYPE m_lastConfirmedAttackType = GENERAL_ATTACK_TYPE_LIGHT;

	// 캐싱된 텍스처 리소스
	std::shared_ptr<TextureResource> m_normalTexResource;
	std::shared_ptr<TextureResource> m_hoverTexResource;
	std::shared_ptr<TextureResource> m_lightAttackTexResource;
	std::shared_ptr<TextureResource> m_strongAttackTexResource;
	std::shared_ptr<TextureResource> m_areaAttackTexResource;
	std::shared_ptr<TextureResource> m_disarmTexResource;

	// 리스너 목록
	std::vector<StanceChangeListener> m_listeners;

	// Depth Sorting
	std::vector<std::pair<HandleOf<ImageUIComponent>, int32_t>> m_managedImages;
	std::vector<std::pair<HandleOf<ButtonUIComponent>, int32_t>> m_managedButtons;

private:
	// 헬퍼 함수 선언
	void CreateAndSetupUI();
	void SetChildUIPositions(float scale = 1.0f); // 스케일 인자 추가
	void ProcessMouseInput();
	GENERAL_ATTACK_DIR_TYPE CalculateGuardDirection(float deltaX, float deltaY) const;
	//void UpdateUISelection(GENERAL_ATTACK_DIR_TYPE selectedDir, std::optional<GENERAL_ATTACK_TYPE> attackType);
	void OnGuardDirectionConfirmed(GENERAL_ATTACK_DIR_TYPE confirmedDir, GENERAL_ATTACK_TYPE attackType);
	
	void UpdateUIPosition();
	void NotifyListeners(GENERAL_ATTACK_DIR_TYPE dir, std::optional<GENERAL_ATTACK_TYPE> type);

	void OnStanceChanged(uint8_t stance); // FSM에서 호출하는 스탠스 변경 콜백

	static float NormalizeAngle(float degrees); // 각도 정규화

	bool m_isUIInitialized = false;
	void SetupListener();
};