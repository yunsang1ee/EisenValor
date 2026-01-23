#include "stdafxClientFramework.h"
#include "ButtonUIComponent.h"
#include "RectTransformComponent.h"
#include "ImageUIComponent.h"
#include "Scene.h"
#include "ComponentStorage.h"

void ButtonUIComponent::SetState(ButtonState state)
{
	if (m_state != state)
	{
		m_state = state;
		UpdateVisuals();
	}
}

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

	ButtonState nextState = m_state;

	if (isInside)
	{
		if (isMouseDown)
		{
			// 눌린 상태
			nextState = ButtonState::Pressed;
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
			nextState = ButtonState::Hover;
		}
		else
		{
			// 마우스만 올려져 있는 상태
			nextState = ButtonState::Hover;
		}
	}
	else
	{
		// 마우스가 영역 밖에 있음
		nextState = ButtonState::Normal;
	}

	// 상태가 변했다면 시각적 업데이트 수행
	if (nextState != m_state)
	{
		m_state = nextState;
		UpdateVisuals();
	}

	return isInside;
}

void ButtonUIComponent::UpdateVisuals()
{
	if (!m_targetImage.IsValid())
		return;

	GameObject* owner = GetGameObject();
	if (!owner) return;

	Scene* scene = owner->GetScene();
	if (!scene) return;

	auto* imageStorage = scene->GetStorage<ImageUIComponent>();
	if (!imageStorage) return;

	ImageUIComponent* imageComp = imageStorage->Get(m_targetImage);
	if (!imageComp) return;

	// ImageUI에게 현재 상태만 전달 (색상은 ImageUI가 알아서 결정)
	imageComp->SetState(m_state);
}