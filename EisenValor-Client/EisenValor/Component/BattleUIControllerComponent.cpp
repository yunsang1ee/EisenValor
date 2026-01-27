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
#include "UI/UITextureGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
#include "CameraComponent.h"
#include <algorithm> // for std::clamp

void BattleUIControllerComponent::OnAttach()
{
	if (auto* owner = GetGameObject())
	{
		owner->SetActive(true);
	}

	InitializeChildHandlesAndSetupUI();

	// 1. NEUTRAL -> UI 숨김
	m_currentStance = GENERAL_STANCE_TYPE_NEUTRAL;
	ToggleUI(false);
}

void BattleUIControllerComponent::OnUpdate(float deltaTime)
{
	if (!m_upButtonHandle.IsValid() || !m_leftButtonHandle.IsValid() || !m_rightButtonHandle.IsValid())
	{
		InitializeChildHandlesAndSetupUI();
	}

	// 1. Ctrl 키
	if (GLOBAL(InputGlobal).GetInputDown(VK_CONTROL))
	{
		//패킷 주석처리
		//auto pb = NetBridge::C2S::Make_CS_CHANGE_PLAYER_STANCE_PACKET();
		//GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

		if (m_currentStance == GENERAL_STANCE_TYPE_NEUTRAL)
		{
			// 전투 모드
			m_currentStance = GENERAL_STANCE_TYPE_COMBAT;
			ToggleUI(true); 
			DEBUG_LOG_FMT("[BattleUI] Switch to COMBAT\n");
		}
		else
		{
			// 일반 모드
			m_currentStance = GENERAL_STANCE_TYPE_NEUTRAL;
			ToggleUI(false); // 숨김

			// 상태 초기화
			m_accumulatedDeltaX = 0.0f;
			m_accumulatedDeltaY = 0.0f;
			UpdateUISelection(GENERAL_ATTACK_DIR_TYPE_NONE, std::nullopt);

			            // 전투 모드 해제 시 카메라를 로컬 플레이어 추적 모드(자유 회전)로 복귀
			            if (auto* mainCamera = CameraComponent::GetMainCamera())
			            {	
			                // 캐릭터 있을 때
			                if (m_targetTrHandle.IsValid())
			                {
			                    mainCamera->SetLookAtTarget(m_targetTrHandle);
			                    mainCamera->SetEnableLookAtRotation(false);
			                    // 원래 오프셋으로 복구
			                    mainCamera->SetFollowOffsetLocal({1.0f, 1.0f, -5.0f});
			                    DEBUG_LOG_FMT("[BattleUI] Switch to NEUTRAL & Reset Camera to LocalPlayer\n");
			                }
			
			                // 죽었을 때(일단 Clear로 해둠)
			                // TODO: 죽었을 때 처리
			                else
			                {
			                    mainCamera->ClearLookAtTarget();
			                    DEBUG_LOG_FMT("[BattleUI] Switch to NEUTRAL & Clear Camera Target\n");
			                }
			            }		}
	}

	// 2. COMBAT 모드 일때만 로직 수행
	if (m_currentStance == GENERAL_STANCE_TYPE_COMBAT)
	{
		// 피드백 타이머 감소 (시간 기반)
		if (m_attackFeedbackTimer > 0.0f)
		{
			m_attackFeedbackTimer -= deltaTime;
			if (m_attackFeedbackTimer <= 0.0f)
			{
				m_attackFeedbackTimer = 0.0f;
				// 타이머 종료 시 UI 갱신 (선택 해제)
				UpdateUISelection(m_currentSelectedDir, std::nullopt);
			}
		}

		// Alt 키: 카메라 락온 타겟 변경 요청
		if (GLOBAL(InputGlobal).GetInputDown(VK_MENU))
		{
			auto pb = NetBridge::C2S::Make_CS_CHANGE_CAMERA_TARGET_PACKET(0);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}

		ProcessMouseInput();
		UpdatePositionFromTarget();
	}
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
	// 1. UI 비활성화
	ToggleUI(false);

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

	// 2. 모든 자식 핸들이 유효한지 확인
	if (m_upButtonHandle.IsValid() && m_leftButtonHandle.IsValid() && m_rightButtonHandle.IsValid())
	{
		// 처음 연결되었을 때 현재 상태(Neutral)에 맞춰 비활성화
		ToggleUI(m_currentStance == GENERAL_STANCE_TYPE_COMBAT);

		// DEBUG_LOG_FMT("[BattleUI] Children Linked Successfully\n");
		SetChildUIPositions();

		// 텍스처 로드 및 멤버 변수 저장
		auto& texGlobal = UITextureGlobal::GetInstance();
		m_normalTexId = texGlobal.LoadTexture(L"Resource\\Texture\\normal.dds");
		m_hoverTexId = texGlobal.LoadTexture(L"Resource\\Texture\\hovering.dds");
		m_lightAttackTexId = texGlobal.LoadTexture(L"Resource\\Texture\\select.dds");
		m_strongAttackTexId = texGlobal.LoadTexture(L"Resource\\Texture\\strong.dds");
		m_areaAttackTexId = texGlobal.LoadTexture(L"Resource\\Texture\\area.dds");
		m_disarmTexId = texGlobal.LoadTexture(L"Resource\\Texture\\disarm.dds");

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
		// DEBUG_LOG_FMT("[BattleUI] No Target Handle!\n");
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
	{
		// 거리 반비례
		scale = kReferenceDistance / distance;
		scale = std::clamp(scale, kMinScale, kMaxScale);
	}

	SetChildUIPositions(scale);

	// 5. Project
	DirectX::XMVECTOR screenPos =
		DirectX::XMVector3Project(worldPos, 0.0f, 0.0f, width, height, 0.0f, 1.0f, proj, view, world);

	DirectX::XMFLOAT3 pos;
	DirectX::XMStoreFloat3(&pos, screenPos);

	// 6. 카메라 뒤에 있는 경우 처리 (Z-range [0, 1] 체크)
	if (pos.z < 0.0f || pos.z > 1.0f)
	{
		ToggleUI(false);
	}
	else
	{
		ToggleUI(true);
	}

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

GENERAL_ATTACK_DIR_TYPE BattleUIControllerComponent::CalculateGuardDirection(float deltaX, float deltaY) const
{
	float lengthSq = (deltaX * deltaX) + (deltaY * deltaY);
	if (lengthSq < kAccumulationThresholdSq)
		return GENERAL_ATTACK_DIR_TYPE_NONE;

	float radian = atan2f(-deltaY, deltaX);
	float degree = radian * kRadToDeg;
	degree = NormalizeAngle(degree);

	if (degree >= kUpRegionStart && degree < kUpRegionEnd)
		return GENERAL_ATTACK_DIR_TYPE_TOP;
	if (degree >= kLeftRegionStart && degree < kLeftRegionEnd)
		return GENERAL_ATTACK_DIR_TYPE_LEFT;

	// 리셋 구역 (250~270)
	if (degree >= kDeadZoneRegionStart && degree < kDeadZoneRegionEnd)
		return GENERAL_ATTACK_DIR_TYPE_NONE;

	if (degree >= kRightRegionStart || degree < kRightRegionEnd)
		return GENERAL_ATTACK_DIR_TYPE_RIGHT;

	return GENERAL_ATTACK_DIR_TYPE_NONE;
}

void BattleUIControllerComponent::ProcessMouseInput()
{
	auto& input = GLOBAL(InputGlobal);
	DX::XMFLOAT2 mouseDelta = input.GetMouseDelta();

	if ((mouseDelta.x * mouseDelta.x + mouseDelta.y * mouseDelta.y) > kMouseDeltaIgnoreSq)
	{
		float instantRadian = atan2f(-mouseDelta.y, mouseDelta.x);
		float instantDegree = NormalizeAngle(instantRadian * kRadToDeg);

		GENERAL_ATTACK_DIR_TYPE instantDir = GENERAL_ATTACK_DIR_TYPE_NONE;
		if (instantDegree >= kUpRegionStart && instantDegree < kUpRegionEnd)
			instantDir = GENERAL_ATTACK_DIR_TYPE_TOP;
		else if (instantDegree >= kLeftRegionStart && instantDegree < kLeftRegionEnd)
			instantDir = GENERAL_ATTACK_DIR_TYPE_LEFT;
		else if (instantDegree >= kDeadZoneRegionStart && instantDegree < kDeadZoneRegionEnd)
			instantDir = GENERAL_ATTACK_DIR_TYPE_NONE;
		else if (instantDegree >= kRightRegionStart || instantDegree < kRightRegionEnd)
			instantDir = GENERAL_ATTACK_DIR_TYPE_RIGHT;

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
		UpdateUISelection(GENERAL_ATTACK_DIR_TYPE_NONE, std::nullopt);
		return;
	}

	// 2. 방향 판정
	GENERAL_ATTACK_DIR_TYPE detectedDir = CalculateGuardDirection(m_accumulatedDeltaX, m_accumulatedDeltaY);

	bool isLeftPressed = input.GetInput(VK_LBUTTON);
	bool isRightPressed = input.GetInput(VK_RBUTTON);
	bool isMiddlePressed = input.GetInput(VK_MBUTTON);
	
	std::optional<GENERAL_ATTACK_TYPE> currentType = std::nullopt;
	
	if (isMiddlePressed) currentType = GENERAL_ATTACK_TYPE_DISARM;
	else if (isLeftPressed && isRightPressed) currentType = GENERAL_ATTACK_TYPE_AREA;
	else if (isRightPressed) currentType = GENERAL_ATTACK_TYPE_HEAVY;
	else if (isLeftPressed) currentType = GENERAL_ATTACK_TYPE_LIGHT;
	
	// 3. 새로운 유효 방향, 공격 타입이 감지되었을 때만 업데이트 및 누적치 초기화
	if (detectedDir != GENERAL_ATTACK_DIR_TYPE_NONE)
	{
		if (detectedDir != m_currentSelectedDir || currentType != m_currentAttackType)
		{
			if (detectedDir != m_currentSelectedDir)
			{
				m_accumulatedDeltaX = 0.0f;
				m_accumulatedDeltaY = 0.0f;

				// 방향이 바뀔 때 패킷 전송 (단순 방향 표시용)
				auto pb = NetBridge::C2S::Make_CS_SHOW_PLAYER_ATTACK_DIR_PACKET(detectedDir);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

				// 마우스를 누른 채 이동한 경우 무효화
				m_isAttackValid = false;
			}
			UpdateUISelection(detectedDir, currentType);
		}
	}
	else if (m_currentSelectedDir != GENERAL_ATTACK_DIR_TYPE_NONE && currentType != m_currentAttackType)
	{
		// 방향은 그대로인데 공격 타입(눌림 상태)만 바뀐 경우
		UpdateUISelection(m_currentSelectedDir, currentType);
	}

	bool isLeftDown = input.GetInputDown(VK_LBUTTON);
	bool isRightDown = input.GetInputDown(VK_RBUTTON);
	bool isMiddleDown = input.GetInputDown(VK_MBUTTON);
	
	// 4. 마우스 클릭 (공격 확정) - Vaild일 때만
	
	if ((isLeftDown || isRightDown || isMiddleDown) && m_isAttackValid)
	{
		if (m_currentSelectedDir != GENERAL_ATTACK_DIR_TYPE_NONE)
		{
			GENERAL_ATTACK_TYPE confirmedType = GENERAL_ATTACK_TYPE_LIGHT;
			if (isMiddleDown) confirmedType = GENERAL_ATTACK_TYPE_DISARM;
			else if ((isLeftDown || isLeftPressed) && (isRightDown || isRightPressed)) confirmedType = GENERAL_ATTACK_TYPE_AREA;
			else if (isRightDown) confirmedType = GENERAL_ATTACK_TYPE_HEAVY;

			OnGuardDirectionConfirmed(m_currentSelectedDir, confirmedType);
			
			// 공격 패킷 전송
			FB_STRUCTS::GeneralAttackInfo attackInfo(confirmedType, m_currentSelectedDir);
			auto pb = NetBridge::C2S::Make_CS_PLAYER_ATTACK_PACKET(&attackInfo);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

			// 확정 후 즉시 초기화
			m_accumulatedDeltaX = 0.0f;
			m_accumulatedDeltaY = 0.0f;

			// 공격 피드백 설정 (입력 무효화 + 텍스쳐 잔상)
			m_isAttackValid = false;
			m_attackFeedbackTimer = 10.0f / 60.0f;
			m_lastConfirmedAttackType = confirmedType;
		}
	}
	else if (!isLeftPressed && !isRightPressed && !isMiddlePressed)
	{
		// 마우스 버튼을 모두 뗐을 때 공격 유효성 리셋
		m_isAttackValid = true;

		if (m_accumulatedDeltaX * m_accumulatedDeltaX + m_accumulatedDeltaY * m_accumulatedDeltaY < kMouseDeltaIgnoreSq)
		{
			m_accumulatedDeltaX = 0.0f;
			m_accumulatedDeltaY = 0.0f;
		}
	}
}

void BattleUIControllerComponent::UpdateUISelection(GENERAL_ATTACK_DIR_TYPE selectedDir, std::optional<GENERAL_ATTACK_TYPE> attackType)
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

	// 공격 유효성이 false - 확정 텍스쳐 안 보여줌 (타이머가 남아있으면 유지)
	if (!m_isAttackValid)
	{
		if (m_attackFeedbackTimer > 0.0f)
		{
			attackType = m_lastConfirmedAttackType;
		}
		else
		{
			attackType = std::nullopt;
		}
	}

	auto updateBtn = [&](HandleOf<ButtonUIComponent> btnHandle, HandleOf<ImageUIComponent> imgHandle, bool isSelected)
	{
		// 1. 텍스처 업데이트
		if (imgHandle.IsValid())
		{
			if (ImageUIComponent* img = imgStorage->Get(imgHandle))
			{
				// 공격 타입에 따라 Pressed 텍스처 선택 (기본 Light)
				uint32_t pressedTexId = m_lightAttackTexId;
				if (attackType.has_value())
				{
					if (attackType.value() == GENERAL_ATTACK_TYPE_HEAVY)
					{
						pressedTexId = m_strongAttackTexId;
					}
					else if (attackType.value() == GENERAL_ATTACK_TYPE_AREA)
					{
						pressedTexId = m_areaAttackTexId;
					}
					else if (attackType.value() == GENERAL_ATTACK_TYPE_DISARM)
					{
						pressedTexId = m_disarmTexId;
					}
				}
				img->SetPressedTexture(pressedTexId);
			}
		}

		// 2. 버튼 상태 업데이트
		if (btnHandle.IsValid())
		{
			if (ButtonUIComponent* btn = btnStorage->Get(btnHandle))
			{
				if (isSelected)
				{
					// attackType이 값이 있으면 Pressed, 아니면 Hover
					bool isPressed = attackType.has_value();
					btn->SetState(isPressed ? ButtonState::Pressed : ButtonState::Hover);
				}
				else
				{
					btn->SetState(ButtonState::Normal);
				}
			}
		}
	};

	updateBtn(m_upButtonHandle, m_upImageHandle, selectedDir == GENERAL_ATTACK_DIR_TYPE_TOP);
	updateBtn(m_leftButtonHandle, m_leftImageHandle, selectedDir == GENERAL_ATTACK_DIR_TYPE_LEFT);
	updateBtn(m_rightButtonHandle, m_rightImageHandle, selectedDir == GENERAL_ATTACK_DIR_TYPE_RIGHT);

	m_currentSelectedDir = selectedDir;
	m_currentAttackType = attackType;
}

void BattleUIControllerComponent::OnGuardDirectionConfirmed(GENERAL_ATTACK_DIR_TYPE confirmedDir, GENERAL_ATTACK_TYPE attackType)
{
	const char* attackStr = (attackType == GENERAL_ATTACK_TYPE_HEAVY) ? "STRONG" : "LIGHT";
	if (attackType == GENERAL_ATTACK_TYPE_DISARM)
	{
		attackStr = "DISARM";
	}

	switch (confirmedDir)
	{
	case GENERAL_ATTACK_DIR_TYPE_TOP:
		DEBUG_LOG_FMT("[BattleUI] UP confirmed! ({})\n", attackStr);
		break;
	case GENERAL_ATTACK_DIR_TYPE_LEFT:
		DEBUG_LOG_FMT("[BattleUI] LEFT confirmed! ({})\n", attackStr);
		break;
	case GENERAL_ATTACK_DIR_TYPE_RIGHT:
		DEBUG_LOG_FMT("[BattleUI] RIGHT confirmed! ({})\n", attackStr);
		break;
	case GENERAL_ATTACK_DIR_TYPE_NONE:
		break;
	}
}

void BattleUIControllerComponent::ToggleUI(bool isActive)
{
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;

	auto* btnStorage = scene->GetStorage<ButtonUIComponent>();
	if (!btnStorage) return;

	auto setObjActive = [&](HandleOf<ButtonUIComponent> handle) 
	{
		if (!handle.IsValid()) return; 
		
		if (auto* btn = btnStorage->Get(handle))
		{
			if (auto* go = btn->GetGameObject())
			{
				go->SetActive(isActive);
			}
		}
	};

	setObjActive(m_upButtonHandle);
	setObjActive(m_leftButtonHandle);
	setObjActive(m_rightButtonHandle);
}
