#pragma once
#include <IComponent.h>
#include <GameObject.h>
#include <ImageUIComponent.h>
#include <RectTransformComponent.h>

class VitalUIControllerComponent : public ComponentBase<VitalUIControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "VitalUIControllerComponent"; }

	VitalUIControllerComponent() = default;
	virtual ~VitalUIControllerComponent() = default;

	// IComponent
	void OnStart() override;
	void OnUpdate(float deltaTime);
	void OnDestroy() override;

private:
	// UI 루트 및 하위 요소 핸들
	HandleOf<GameObject> m_rootUI;

	// 자식 오브젝트 핸들 (레이아웃 조절용)
	HandleOf<GameObject> m_flagRootHandle;
	HandleOf<GameObject> m_hpRootHandle;
	HandleOf<GameObject> m_staminaRootHandle;

	// 게이지 업데이트를 위한 이미지 컴포넌트 핸들
	HandleOf<ImageUIComponent> m_hpFill;
	HandleOf<ImageUIComponent> m_staminaFill; // 플레이어일 때만 유효
	HandleOf<ImageUIComponent> m_flagIcon;

	// UI 설정 상수
	static constexpr float kHPBarWidth = 120.0f;
	static constexpr float kHPBarHeight = 15.0f;
	static constexpr float kStaminaBarWidth = 105.0f;
	static constexpr float kStaminaBarHeight = 13.125f;
	static constexpr float kFlagSize = 52.5f;
	static constexpr float kPadding = -1.0f;
    static constexpr float kVerticalOffset = 2.0f;

	bool m_isPlayer = false;
	bool m_flagInitialized = false; // 깃발 초기화 여부

	// Order 관리용 ImageComponent (핸들 + 기본 Order)
	std::vector<std::pair<HandleOf<ImageUIComponent>, int32_t>> m_managedImages;

private:
	// 헬퍼 함수
	void CreateAndSetupUI();
	void SetChildUIPositions(float scale);
	void ToggleUI(bool isActive);
};