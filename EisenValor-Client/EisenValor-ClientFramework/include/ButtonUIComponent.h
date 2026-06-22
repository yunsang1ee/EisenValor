#pragma once
#include "UIComponent.h"
#include "ImageUIComponent.h"
#include <functional>

// 이벤트 및 상호작용 담당
class ButtonUIComponent : public UIComponent<ButtonUIComponent>
{
public:
	static constexpr const char* GetStaticTypeName() 
	{
		return "ButtonUIComponent"; 
	}

	ButtonUIComponent() = default;
	virtual ~ButtonUIComponent() = default;

	// 버튼 클릭 시 호출
	void SetOnClick(std::function<void()> callback) { m_onClick = callback; }
	void SetOnHover(std::function<void()> callback) { m_onHover = callback; }

	// 상태 호출
	ButtonState GetState() const { return m_state; }
	void		SetState(ButtonState state);

	// 타겟 이미지 설정 (시각적 피드백 대상)
	void SetTargetImage(HandleOf<ImageUIComponent> handle) { m_targetImage = handle; }

	// UI Component Override
	// 마우스 위치와 버튼 상태 확인 후 이벤트발생
	bool ProcessInput(const DirectX::XMFLOAT2& mousePos, bool isMouseDown, bool isMouseUp);

	// 렌더링 데이터 생성 - ButtonUI는 직접 렌더링하지 않음
	virtual void GetRenderData(std::vector<UIRenderData>& outData) override {}

private:
	// 상태 변화를 타겟 이미지에 전파
	void UpdateVisuals();

private:
	ButtonState m_state = ButtonState::Normal;

	// 이벤트 콜백
	std::function<void()> m_onClick;
	std::function<void()> m_onHover;

	// 시각적 피드백을 줄 대상 이미지 (색상 로직은 ImageUI가 담당)
	HandleOf<ImageUIComponent> m_targetImage;
};
