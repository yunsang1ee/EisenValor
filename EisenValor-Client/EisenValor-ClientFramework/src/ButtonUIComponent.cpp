#include "stdafxClientFramework.h"
#include "ButtonUIComponent.h"
#include "RectTransformComponent.h"

bool ButtonUIComponent::ProcessInput(const DirectX::XMFLOAT2& mousePos, bool isMouseDown, bool isMouseUp)
{
	// 입력 무시
	if (m_state == ButtonState::Disabled || !IsInteractable())
	{
		return false;
	}

	RectTransformComponent* rectTr = GetRectTransform();
	if (!rectTr)
	{
		return false;
	}

	// 마우스가 버튼 영역 안에 있는지 확인 (Contains 사용)
	Vec2 vMousePos = {mousePos.x, mousePos.y};
	bool isInside = rectTr->Contains(vMousePos);

	if (isInside)
	{
		if (isMouseDown)
		{
			// 눌린 상태
			m_state = ButtonState::Pressed;
			return true;
		}
		else if (isMouseUp)
		{
			// 눌렀다 뗐을 때 (클릭)
			if (m_state == ButtonState::Pressed || m_state == ButtonState::Hover)
			{
				if (m_onClick)
				{
					m_onClick();
				}
			}
			m_state = ButtonState::Hover;
			return true;
		}
		else
		{
			// 마우스만 올려져 있는 상태
			// (기존에 Pressed였다가 밖으로 나가지 않고 버튼 위에서 마우스를 뗀 상태 포함)
			m_state = ButtonState::Hover;
			return true; // TODO: true/false 결정 필요
		}
	}
	else
	{
		// 마우스가 영역 밖에 있음
		m_state = ButtonState::Normal;
		return false;
	}
}

void ButtonUIComponent::GetRenderData(std::vector<UIRenderData>& outData)
{
	RectTransformComponent* rectTr = GetRectTransform();
	if (!rectTr)
	{
		return;
	}

	// 현재 상태에 따른 색상 결정
	DirectX::XMFLOAT4 targetColor = m_normalColor;
	switch (m_state)
	{
	case ButtonState::Normal:
		targetColor = m_normalColor;
		break;
	case ButtonState::Hover:
		targetColor = m_hoverColor;
		break;
	case ButtonState::Pressed:
		targetColor = m_pressedColor;
		break;
	case ButtonState::Disabled:
		targetColor = m_disabledColor;
		break;
	}

	// 색상 Blending
	DirectX::XMFLOAT4 finalColor;
	finalColor.x = m_color.x * targetColor.x;
	finalColor.y = m_color.y * targetColor.y;
	finalColor.z = m_color.z * targetColor.z;
	finalColor.w = m_color.w * targetColor.w;

	// 렌더링 데이터 생성
	UIRenderData data;
	data.textureId = 0; // ButtonUI는 텍스처를 사용하지 않으므로 0으로 설정
	data.rect = rectTr->GetRect();
	data.color = finalColor;
	data.uvMin = {0.0f, 0.0f};
	data.uvMax = {1.0f, 1.0f};

	outData.push_back(data);
}
