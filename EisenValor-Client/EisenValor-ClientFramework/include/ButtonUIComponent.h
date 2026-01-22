#pragma once
#include "UIComponent.h"
#include <functional>

// 버튼 상태
enum class ButtonState
{
	Normal,
	Hover,
	Pressed,
	Disabled
};

// 이벤트
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

	// 상태 호출
	ButtonState GetState() const { return m_state; }
	void		SetState(ButtonState state) { m_state = state; }

	// UI Component Override
	// 마우스 위치와 버튼 상태 확인 후 이벤트발생
	bool ProcessInput(const DirectX::XMFLOAT2& mousePos, bool isMouseDown, bool isMouseUp);

	// 렌더링 데이터 생성
	// 상태에 따라 색상 변경하여 피드백
	virtual void GetRenderData(std::vector<UIRenderData>& outData) override;

private:
	ButtonState m_state = ButtonState::Normal;

	// 이벤트 콜백
	std::function<void()> m_onClick;

	// 상태별 색상
	//
	DirectX::XMFLOAT4 m_normalColor = {0.7f, 0.7f, 0.7f, 1.0f};	// 회색
	DirectX::XMFLOAT4 m_hoverColor = {0.0f, 0.0f, 0.0f, 1.0f};	// 검은색
	DirectX::XMFLOAT4 m_pressedColor = {1.0f, 0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT4 m_disabledColor = {0.0f, 0.0f, 0.0f, 0.0f}; // 반투명 회색
};
