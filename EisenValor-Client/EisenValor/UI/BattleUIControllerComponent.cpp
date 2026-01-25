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
#include "UITextureGlobal.h"
#include <algorithm> // for std::clamp

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
	UpdatePositionFromTarget();
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
	if (!owner)
		return;

	Scene* scene = owner->GetScene();
	if (!scene)
		return;

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
		if (!childTr)
			continue;

		GameObject* childObj = childTr->GetGameObject();
		if (!childObj)
			continue;

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
		DEBUG_LOG_FMT("[BattleUI] Children Linked Successfully!\n");
		SetChildUIPositions();

		// 텍스처 로드 및 멤버 변수 저장
		auto& texGlobal = UITextureGlobal::GetInstance();
		m_normalTexId = texGlobal.LoadTexture(L"Resource\\Texture\\normal.dds");
		m_hoverTexId = texGlobal.LoadTexture(L"Resource\\Texture\\hovering.dds");
		m_lightAttackTexId = texGlobal.LoadTexture(L"Resource\\Texture\\select.dds");
		m_strongAttackTexId = texGlobal.LoadTexture(L"Resource\\Texture\\strong.dds");

		// ButtonUI와 ImageUI 초기 설정
		auto* btnStorage = scene->GetStorage<ButtonUIComponent>();
		auto* imgStorage = scene->GetStorage<ImageUIComponent>();

		auto setupVisuals = [&](HandleOf<ButtonUIComponent> btnHandle, HandleOf<ImageUIComponent> imgHandle)
		{
			if (!btnHandle.IsValid() || !imgHandle.IsValid())
				return;

			ButtonUIComponent* btn = btnStorage->Get(btnHandle);
			ImageUIComponent*  img = imgStorage->Get(imgHandle);
			if (btn && img)
			{
				btn->SetTargetImage(imgHandle);

				// 초기 상태 설정
				img->SetNormalTexture(m_normalTexId);
				img->SetHoverTexture(m_hoverTexId);
				img->SetPressedTexture(m_lightAttackTexId); 
				img->SetDisabledTexture(m_normalTexId);

				// 색상 설정
				img->SetNormalColor({0.7f, 0.7f, 0.7f, 1.0f});
				img->SetHoverColor({0.0f, 0.0f, 0.0f, 1.0f});
				img->SetPressedColor({1.0f, 0.0f, 0.0f, 1.0f});
				img->SetDisabledColor({0.5f, 0.5f, 0.5f, 0.5f});
			}
		};

		setupVisuals(m_upButtonHandle, m_upImageHandle);
		setupVisuals(m_leftButtonHandle, m_leftImageHandle);
		setupVisuals(m_rightButtonHandle, m_rightImageHandle);
	}
}

void BattleUIControllerComponent::SetChildUIPositions(float scale)
{
	GameObject* owner = GetGameObject();
	if (!owner)
		return;
	Scene* scene = owner->GetScene();
	if (!scene)
		return;

	// 반지름 갱신 (스케일 적용)
	m_radius = kDefaultRadius * scale;
	const float innerRadius = m_radius * kInnerRadiusRatio;
	 
	// UI 크기 갱신 (스케일 적용)
	const float currentUISize = kUISize * scale;
	const float currentUIHalfSize = currentUISize / 2.0f;

	auto* btnStorage = scene->GetStorage<ButtonUIComponent>();

	auto setupPos = [&](HandleOf<ButtonUIComponent> handle, float angleDeg)
	{
		if (!handle.IsValid())
			return;
		ButtonUIComponent* btn = btnStorage->Get(handle);
		if (!btn)
			return;

		RectTransformComponent* rectTr = btn->GetGameObject()->GetComponent<RectTransformComponent>();
		if (!rectTr)
			return;

		float offsetX = cosf(angleDeg * kDegToRad) * innerRadius;
		float offsetY = -sinf(angleDeg * kDegToRad) * innerRadius;

		rectTr->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
		rectTr->SetPivot({0.5f, 0.5f});

		rectTr->SetSizeDelta({currentUISize, currentUISize});
		rectTr->SetOffsetMin({offsetX - currentUIHalfSize, offsetY - currentUIHalfSize});
		rectTr->SetOffsetMax({offsetX + currentUIHalfSize, offsetY + currentUIHalfSize});
	};

	setupPos(m_upButtonHandle, 90.0f);
	setupPos(m_leftButtonHandle, 210.0f);
	setupPos(m_rightButtonHandle, 330.0f);
}

void BattleUIControllerComponent::UpdatePositionFromTarget()
{
	if (!m_targetTrHandle.IsValid())
	{
		DEBUG_LOG_FMT("[BattleUI] No Target Handle!\n");
		return;
	}

	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;

	Transform* targetTr = scene->GetStorage<Transform>()->Get(m_targetTrHandle);
	if (!targetTr)
	{
		DEBUG_LOG_FMT("[BattleUI] Target Transform is NULL!\n");
		return;
	}

	// 1. 타겟의 월드 위치 가져오기
	DirectX::XMFLOAT3 vPos = targetTr->GetWorldPosition();
	DirectX::XMVECTOR worldPos = DirectX::XMLoadFloat3(&vPos);

	// 2. 뷰포트 정보 가져오기
	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
	if (!swapChain) return;

	float width = static_cast<float>(swapChain->GetWidth());
	float height = static_cast<float>(swapChain->GetHeight());

	// 3. 카메라 행렬 가져오기
	DirectX::XMMATRIX view = CameraComponent::GetMainViewMatrix();
	DirectX::XMMATRIX proj = CameraComponent::GetMainProjectionMatrix();
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

	// 4. 거리 비례 스케일링 계산(Z)
	DirectX::XMVECTOR viewPos = DirectX::XMVector3TransformCoord(worldPos, view);
	float distance = DirectX::XMVectorGetZ(viewPos);

	float scale = 1.0f;
	if (distance > 0.1f) 
	{	// 거리 반비례
		scale = kReferenceDistance / distance;
		scale = std::clamp(scale, kMinScale, kMaxScale);
	}

	SetChildUIPositions(scale);

	// 5. Project
	DirectX::XMVECTOR screenPos =
		DirectX::XMVector3Project(worldPos, 0.0f, 0.0f, width, height, 0.0f, 1.0f, proj, view, world);

	DirectX::XMFLOAT3 pos;
	DirectX::XMStoreFloat3(&pos, screenPos);

	// 자식 UI 요소들 표시/비표시 제어 헬퍼
	auto setChildrenActive = [&](bool active)
	{
		auto setUIActive = [&](HandleOf<ButtonUIComponent> handle)
		{
			if (handle.IsValid())
			{
				if (auto* btn = scene->GetStorage<ButtonUIComponent>()->Get(handle))
				{
					if (auto* go = btn->GetGameObject()) go->SetActive(active);
				}
			}
		};
		setUIActive(m_upButtonHandle);
		setUIActive(m_leftButtonHandle);
		setUIActive(m_rightButtonHandle);
	};

	// 6. 카메라 뒤에 있는 경우 처리 (Z-range [0, 1] 체크)
	if (pos.z < 0.0f || pos.z > 1.0f)
	{
		setChildrenActive(false); 
	}

	setChildrenActive(true);

	// 7. 중심점 갱신 및 BattleUI 오브젝트 자체의 위치 이동
	m_centerX = pos.x;
	m_centerY = pos.y;

	if (auto* rectTr = owner->GetComponent<RectTransformComponent>()) 
	{	
		// 화면 좌표를 UI 오프셋으로 변환 (Anchor 0.5,0.5기준)
		float offsetX = pos.x - (width * 0.5f);
		float offsetY = pos.y - (height * 0.5f);

		// 메인 컨테이너 영역 스케일링
		float containerHalfSize = 50.0f * scale; 

		rectTr->SetOffsetMin({offsetX - containerHalfSize, offsetY - containerHalfSize});
		rectTr->SetOffsetMax({offsetX + containerHalfSize, offsetY + containerHalfSize});
	}
}

EGuardDir BattleUIControllerComponent::CalculateGuardDirection(float deltaX, float deltaY) const
{
	float lengthSq = (deltaX * deltaX) + (deltaY * deltaY);
	if (lengthSq < kAccumulationThresholdSq)
		return EGuardDir::None;

	float radian = atan2f(-deltaY, deltaX);
	float degree = radian * kRadToDeg;
	degree = NormalizeAngle(degree);

	if (degree >= kUpRegionStart && degree < kUpRegionEnd)
		return EGuardDir::Up;
	if (degree >= kLeftRegionStart && degree < kLeftRegionEnd)
		return EGuardDir::Left;

	// 리셋 구역 (250~270)
	if (degree >= kDeadZoneRegionStart && degree < kDeadZoneRegionEnd)
		return EGuardDir::None;

	if (degree >= kRightRegionStart || degree < kRightRegionEnd)
		return EGuardDir::Right;

	return EGuardDir::None;
}

void BattleUIControllerComponent::ProcessMouseInput()
{
	auto& input = GLOBAL(InputGlobal);
	DX::XMFLOAT2 mouseDelta = input.GetMouseDelta();

	if ((mouseDelta.x * mouseDelta.x + mouseDelta.y * mouseDelta.y) > kMouseDeltaIgnoreSq)
	{
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

		if (instantDir != m_currentSelectedDir)
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
		UpdateUISelection(EGuardDir::None, EAttackType::None);
		return;
	}

	// 2. 방향 판정
	EGuardDir detectedDir = CalculateGuardDirection(m_accumulatedDeltaX, m_accumulatedDeltaY);

	bool isLeftPressed = input.GetInput(VK_LBUTTON);
	bool isRightPressed = input.GetInput(VK_RBUTTON);
	EAttackType currentType = EAttackType::None;

	if (isLeftPressed) currentType = EAttackType::Light;
	else if (isRightPressed) currentType = EAttackType::Strong;
	
	// 3. 새로운 유효 방향, 공격 타입이 감지되었을 때만 업데이트 및 누적치 초기화
	if (detectedDir != EGuardDir::None)
	{
		if (detectedDir != m_currentSelectedDir || currentType != m_currentAttackType)
		{
			if (detectedDir != m_currentSelectedDir)
			{
				m_accumulatedDeltaX = 0.0f;
				m_accumulatedDeltaY = 0.0f;
			}
			UpdateUISelection(detectedDir, currentType);
		}
	}
	else if (m_currentSelectedDir != EGuardDir::None && currentType != m_currentAttackType)
	{
		UpdateUISelection(m_currentSelectedDir, currentType);
	}

	bool isLeftDown = input.GetInputDown(VK_LBUTTON);
	bool isRightDown = input.GetInputDown(VK_RBUTTON);
	
	// 4. 마우스 좌클릭
	if (isLeftDown || isRightDown)
	{
		if (m_currentSelectedDir != EGuardDir::None)
		{
			EAttackType confirmedType = isLeftDown ? EAttackType::Light : EAttackType::Strong;
			OnGuardDirectionConfirmed(m_currentSelectedDir, confirmedType);
			
			// 확정 후 즉시 초기화
			m_accumulatedDeltaX = 0.0f;
			m_accumulatedDeltaY = 0.0f;
		}
	}
	else if (!isLeftPressed && !isRightPressed)
	{
		if (m_accumulatedDeltaX * m_accumulatedDeltaX + m_accumulatedDeltaY * m_accumulatedDeltaY < kMouseDeltaIgnoreSq)
		{
			m_accumulatedDeltaX = 0.0f;
			m_accumulatedDeltaY = 0.0f;
		}
	}
}

void BattleUIControllerComponent::UpdateUISelection(EGuardDir selectedDir, EAttackType attackType)
{
	GameObject* owner = GetGameObject();
	if (!owner)
		return;
	Scene* scene = owner->GetScene();
	if (!scene)
		return;

	auto* btnStorage = scene->GetStorage<ButtonUIComponent>();
	if (!btnStorage)
		return;
	auto* imgStorage = scene->GetStorage<ImageUIComponent>();

	auto updateBtn = [&](HandleOf<ButtonUIComponent> btnHandle, HandleOf<ImageUIComponent> imgHandle, bool isSelected)
	{
		// 1. 텍스처 업데이트
		if (imgHandle.IsValid())
		{
			if (ImageUIComponent* img = imgStorage->Get(imgHandle))
			{
				// 공격 타입에 따라 Pressed 텍스처 선택
				img->SetPressedTexture(attackType == EAttackType::Strong ? m_strongAttackTexId : m_lightAttackTexId);
			}
		}

		// 2. 버튼 상태 업데이트
		if (btnHandle.IsValid())
		{
			if (ButtonUIComponent* btn = btnStorage->Get(btnHandle))
			{
				if (isSelected)
				{
					// 눌려 있으면 Pressed 아니면 Hover
					// attackType이 None이 아니면 눌린 상태
					bool isPressed = (attackType != EAttackType::None);
					btn->SetState(isPressed ? ButtonState::Pressed : ButtonState::Hover);
				}
				else
				{
					btn->SetState(ButtonState::Normal);
				}
			}
		}
	};

	updateBtn(m_upButtonHandle, m_upImageHandle, selectedDir == EGuardDir::Up);
	updateBtn(m_leftButtonHandle, m_leftImageHandle, selectedDir == EGuardDir::Left);
	updateBtn(m_rightButtonHandle, m_rightImageHandle, selectedDir == EGuardDir::Right);

	m_currentSelectedDir = selectedDir;
	m_currentAttackType = attackType;
}

void BattleUIControllerComponent::OnGuardDirectionConfirmed(EGuardDir confirmedDir, EAttackType attackType)
{
	const char* attackStr = (attackType == EAttackType::Strong) ? "STRONG" : "LIGHT";

	switch (confirmedDir)
	{
	case EGuardDir::Up:
		DEBUG_LOG_FMT("[BattleUI] UP confirmed! ({})\n", attackStr);
		break;
	case EGuardDir::Left:
		DEBUG_LOG_FMT("[BattleUI] LEFT confirmed! ({})\n", attackStr);
		break;
	case EGuardDir::Right:
		DEBUG_LOG_FMT("[BattleUI] RIGHT confirmed! ({})\n", attackStr);
		break;
	case EGuardDir::None:
		break;
	}
}
