#include "stdafxClient.h"
#include "BattleUIControllerComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "InputGlobal.h" 
#include "DxRendererGlobal.h" 
#include "DxSwapChain.h" 
#include "RectTransformComponent.h" 
#include "ComponentStorage.h"

void BattleUIControllerComponent::OnAttach()
{
	if (auto* owner = GetGameObject())
	{
		owner->SetActive(true);
	}

	InitializeChildHandlesAndSetupUI();

	GLOBAL(InputGlobal).SetMouseLocked(true);
}

void BattleUIControllerComponent::OnUpdate(float deltaTime)
{
	if (!m_upButtonHandle.IsValid() || !m_leftButtonHandle.IsValid() || !m_rightButtonHandle.IsValid())
	{
		InitializeChildHandlesAndSetupUI();
	}

	ProcessMouseInput();
}

void BattleUIControllerComponent::OnDetach()
{
	GLOBAL(InputGlobal).SetMouseLocked(false);
}

float BattleUIControllerComponent::NormalizeAngle(float degrees)
{
	while (degrees < 0.0f)
		degrees += 360.0f;
	while (degrees >= 360.0f)
		degrees -= 360.0f;
	return degrees;
}

void BattleUIControllerComponent::InitializeChildHandlesAndSetupUI()
{
	GameObject* owner = GetGameObject();
	if (!owner) return;

	Scene* scene = owner->GetScene();
	if (!scene) return;

	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
	if (swapChain)
	{
		m_centerX = static_cast<float>(swapChain->GetWidth()) / 2.0f;
		m_centerY = static_cast<float>(swapChain->GetHeight()) / 2.0f;
	}

	Transform& ownerTr = owner->GetTransform();
	const auto& childrenTrHandles = ownerTr.GetChildren();
	auto* trStorage = scene->GetStorage<Transform>();
	
	for (auto childTrHandle : childrenTrHandles)
	{
		Transform* childTr = trStorage->Get(childTrHandle);
		if (!childTr) continue;

		GameObject* childObj = childTr->GetGameObject();
		if (!childObj) continue;

		std::string name = childObj->GetName();

		if (name == "UpUI")
		{
			m_upButtonHandle = childObj->GetComponentHandle<ButtonUIComponent>();
			m_upImageHandle = childObj->GetComponentHandle<ImageUIComponent>();
		}
		else if (name == "LeftUI")
		{
			m_leftButtonHandle = childObj->GetComponentHandle<ButtonUIComponent>();
			m_leftImageHandle = childObj->GetComponentHandle<ImageUIComponent>();
		}
		else if (name == "RightUI")
		{
			m_rightButtonHandle = childObj->GetComponentHandle<ButtonUIComponent>();
			m_rightImageHandle = childObj->GetComponentHandle<ImageUIComponent>();
		}
	}

	if (m_upButtonHandle.IsValid() && m_leftButtonHandle.IsValid() && m_rightButtonHandle.IsValid())
	{
		SetChildUIPositions();
	}
}

void BattleUIControllerComponent::SetChildUIPositions()
{
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;

	const float innerRadius = m_radius * kInnerRadiusRatio;
	auto* btnStorage = scene->GetStorage<ButtonUIComponent>();

	auto setupPos = [&](HandleOf<ButtonUIComponent> handle, float angleDeg) {
		if (!handle.IsValid()) return;
		ButtonUIComponent* btn = btnStorage->Get(handle);
		if (!btn) return;
		
		RectTransformComponent* rectTr = btn->GetGameObject()->GetComponent<RectTransformComponent>();
		if (!rectTr) return;

		float offsetX = cosf(angleDeg * kDegToRad) * innerRadius;
		float offsetY = -sinf(angleDeg * kDegToRad) * innerRadius;

		rectTr->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
		rectTr->SetPivot({0.5f, 0.5f});
		
		rectTr->SetSizeDelta({kUISize, kUISize});
		rectTr->SetOffsetMin({offsetX - kUIHalfSize, offsetY - kUIHalfSize});
		rectTr->SetOffsetMax({offsetX + kUIHalfSize, offsetY + kUIHalfSize});
	};

	setupPos(m_upButtonHandle, 90.0f);    
	setupPos(m_leftButtonHandle, 210.0f); 
	setupPos(m_rightButtonHandle, 330.0f);
}

EGuardDir BattleUIControllerComponent::CalculateGuardDirection(float deltaX, float deltaY) const
{
	float lengthSq = (deltaX * deltaX) + (deltaY * deltaY);
	if (lengthSq < kAccumulationThresholdSq) return EGuardDir::None;

	float radian = atan2f(-deltaY, deltaX); 
	float degree = radian * kRadToDeg;
	degree = NormalizeAngle(degree);

	if (degree >= kUpRegionStart && degree < kUpRegionEnd) return EGuardDir::Up;
	if (degree >= kLeftRegionStart && degree < kLeftRegionEnd) return EGuardDir::Left;
	
	// 리셋 구역 (250~270)
	if (degree >= kDeadZoneRegionStart && degree < kDeadZoneRegionEnd) return EGuardDir::None;

	if (degree >= kRightRegionStart || degree < kRightRegionEnd) return EGuardDir::Right;

	return EGuardDir::None;
}

void BattleUIControllerComponent::ProcessMouseInput()
{
	auto&		 input = GLOBAL(InputGlobal);
	DX::XMFLOAT2 mouseDelta = input.GetMouseDelta();

	// 움직임이 있을 때
	if ((mouseDelta.x * mouseDelta.x + mouseDelta.y * mouseDelta.y) > kMouseDeltaIgnoreSq)
	{
		// 현재 마우스 델타 자체의 방향을 즉시 판정
		float instantRadian = atan2f(-mouseDelta.y, mouseDelta.x);
		float instantDegree = NormalizeAngle(instantRadian * kRadToDeg);

		EGuardDir instantDir = EGuardDir::None;
		if (instantDegree >= kUpRegionStart && instantDegree < kUpRegionEnd)
			instantDir = EGuardDir::Up;
		else if (instantDegree >= kLeftRegionStart && instantDegree < kLeftRegionEnd)
			instantDir = EGuardDir::Left;
		else if (instantDegree >= kDeadZoneRegionStart && instantDegree < kDeadZoneRegionEnd)
			instantDir = EGuardDir::None;
		else if (instantDegree >= kRightRegionStart || instantDegree < kRightRegionEnd)
			instantDir = EGuardDir::Right;

		// 이미 해당 방향이 호버링된 상태라면 누적하지 않음
		if (instantDir == m_currentSelectedDir)
		{
			// 동일 방향으로 더 이동함 -> 무시
		}
		else
		{
			m_accumulatedDeltaX += mouseDelta.x;
			m_accumulatedDeltaY += mouseDelta.y;
		}
	}

	float totalDeltaSq = (m_accumulatedDeltaX * m_accumulatedDeltaX) + (m_accumulatedDeltaY * m_accumulatedDeltaY);
	float radian = atan2f(-m_accumulatedDeltaY, m_accumulatedDeltaX);
	float degree = NormalizeAngle(radian * kRadToDeg);

	// 1. 데드존
	if (degree >= kDeadZoneRegionStart && degree < kDeadZoneRegionEnd && totalDeltaSq >= kAccumulationThresholdSq)
	{
		m_accumulatedDeltaX = 0.0f;
		m_accumulatedDeltaY = 0.0f;
		UpdateUISelection(EGuardDir::None); // 시각적 호버링 해제
		DEBUG_LOG_FMT("[BattleUI] Input & Hover Reset (Dead Zone)!");
		return;
	}

	// 2. 방향 판정
	EGuardDir detectedDir = CalculateGuardDirection(m_accumulatedDeltaX, m_accumulatedDeltaY);
	
	// 3. 새로운 유효 방향이 감지되었을 때만 업데이트 및 누적치 초기화
	if (detectedDir != EGuardDir::None && detectedDir != m_currentSelectedDir)
	{
		m_accumulatedDeltaX = 0.0f;
		m_accumulatedDeltaY = 0.0f;
		UpdateUISelection(detectedDir);
		DEBUG_LOG_FMT("[BattleUI] Direction Changed -> {}\n", (int)detectedDir);
	}

	// 4. 마우스 좌클릭
	if (input.GetInputDown(VK_LBUTTON))
	{
		UpdateUISelection(m_currentSelectedDir);
		if (m_currentSelectedDir != EGuardDir::None)
		{
			OnGuardDirectionConfirmed(m_currentSelectedDir);
		}
		m_accumulatedDeltaX = 0.0f;
		m_accumulatedDeltaY = 0.0f;
	}
	else if (!input.GetInput(VK_LBUTTON))
	{
		if (m_accumulatedDeltaX * m_accumulatedDeltaX + m_accumulatedDeltaY * m_accumulatedDeltaY < kMouseDeltaIgnoreSq)
		{
			m_accumulatedDeltaX = 0.0f;
			m_accumulatedDeltaY = 0.0f;
		}
	}
}

void BattleUIControllerComponent::UpdateUISelection(EGuardDir selectedDir)
{
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;

	auto* btnStorage = scene->GetStorage<ButtonUIComponent>();
	if (!btnStorage) return;

	// 현재 마우스 왼쪽 버튼이 눌려 있는지 확인
	bool isPressed = GLOBAL(InputGlobal).GetInput(VK_LBUTTON);
	auto updateBtn = [&](HandleOf<ButtonUIComponent> handle, bool isSelected)
	{
		if (handle.IsValid())
		{
			if (ButtonUIComponent* btn = btnStorage->Get(handle))
			{
				if (isSelected)
				{
					// 선택된 방향인데 마우스가 눌려 있다면 Pressed, 아니면 Hover
					btn->SetState(isPressed ? ButtonState::Pressed : ButtonState::Hover);
				}
				else
				{
					btn->SetState(ButtonState::Normal);
				}
			}
		}
	};

	updateBtn(m_upButtonHandle, selectedDir == EGuardDir::Up);
	updateBtn(m_leftButtonHandle, selectedDir == EGuardDir::Left);
	updateBtn(m_rightButtonHandle, selectedDir == EGuardDir::Right);

	m_currentSelectedDir = selectedDir;
}

void BattleUIControllerComponent::OnGuardDirectionConfirmed(EGuardDir confirmedDir)
{
	switch (confirmedDir)
	{
	case EGuardDir::Up:
		DEBUG_LOG_FMT("[BattleUI] UP confirmed!\n");
		break;
	case EGuardDir::Left:
		DEBUG_LOG_FMT("[BattleUI] LEFT confirmed!\n");
		break;
	case EGuardDir::Right:
		DEBUG_LOG_FMT("[BattleUI] RIGHT confirmed!\n");
		break;
	case EGuardDir::None:
		break;
	}
}
