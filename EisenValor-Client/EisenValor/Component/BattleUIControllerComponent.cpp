#include "stdafxClient.h"
#include "BattleUIControllerComponent.h"
#include "HealthComponent.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "GameObject.h"
#include "Transform.h"
#include "InputGlobal.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"
#include "RectTransformComponent.h"
#include "ComponentStorage.h"
#include "ResourceGlobal.h"
#include "TextureResource.h"
#include "DxTexture.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
#include "CameraComponent.h"
#include "Component/FSM/FSMComponent.h"
#include <algorithm> // for std::clamp

void BattleUIControllerComponent::OnAttach()
{
	GameObject* owner = GetGameObject();
	if (owner)
	{
		owner->SetActive(true);
		DEBUG_LOG_FMT("[BattleUI Debug] OnAttach Called! This: {}, Owner: {}, ServerID: {}\n", 
			(void*)this, (void*)owner, owner->GetServerID());
	}
}

void BattleUIControllerComponent::OnStart()
{
	// 이미 초기화된 경우 중복 실행 방지
	if (m_uiRootObjHandle.IsValid() && GetGameObject()->GetScene()->TryGetGameObject(m_uiRootObjHandle))
	{
		return;
	}

	GameObject* owner = GetGameObject();
	//DEBUG_LOG_FMT(
	//	"[BattleUI Debug] OnStart Called! ID: {}, Mode: {}\n", owner->GetServerID(),
	//	m_controlMode == ControlType::Local ? "Local" : "Remote"
	//);
	// UI 동적 생성 및 초기화
	CreateAndSetupUI();

	// InitStance에서 저장된 상태에 따라 UI 갱신
	ToggleUI(m_currentStance == GENERAL_STANCE_TYPE_COMBAT);
}

void BattleUIControllerComponent::OnUpdate(float deltaTime)
{
	// 지연 초기화: 모든 UI 핸들 유효 시 리스너 등록
	if (!m_isUIInitialized)
	{
		// 핸들 유효성 체크
		bool ready = m_uiRootObjHandle.IsValid() &&
					 m_upButtonHandle.IsValid() && m_upImageHandle.IsValid() &&
					 m_leftButtonHandle.IsValid() && m_leftImageHandle.IsValid() &&
					 m_rightButtonHandle.IsValid() && m_rightImageHandle.IsValid();
		
		if (ready)
		{
			SetupListener();
			m_isUIInitialized = true;
			DEBUG_LOG_FMT("[BattleUI] UI Initialized Deferred. Listener Setup Complete.\n");
		}
	}

	GameObject* owner = GetGameObject();
	if (!owner) return;

	// 죽었으면 UI 숨기기
	if (auto* health = owner->GetComponent<HealthComponent>())
	{
		if (health->GetHealth() <= 0)
		{
			ToggleUI(false);
			return;
		}
	}

	// 1. Local 모드일 때만 직접 입력(전투 태세 전환) 처리
	if (m_controlMode == ControlType::Local)
	{
		// 1. Ctrl 키
		if (GLOBAL(InputGlobal).GetInputDown(VK_CONTROL))
		{
			//// 디버그 로그: ID 및 상태 확인
			//if (auto* owner = GetGameObject())
			//{
			//	auto* scene = owner->GetScene();
			//	uint32 localID = scene->GetLocalID();
			//	uint32 ownerID = owner->GetServerID();
			//	bool isRootActive = false;
			//	if (auto* root = scene->TryGetGameObject(m_uiRootObjHandle)) isRootActive = root->IsActive();

			//	DEBUG_LOG_FMT("[BattleUI Debug] Ctrl Pressed! LocalID: {}, OwnerID: {}, RootActive: {}, Stance: {}\n",
			//		localID, ownerID, isRootActive, static_cast<int>(m_currentStance));
			//}

			auto pb = NetBridge::C2S::Make_CS_CHANGE_GENERAL_STANCE_PACKET();
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

			if (m_currentStance == GENERAL_STANCE_TYPE_NEUTRAL)
			{
				// 전투 모드
				SetStance(GENERAL_STANCE_TYPE_COMBAT);
				DEBUG_LOG_FMT("[BattleUI] Switch to COMBAT\n");
			}
			else
			{
				// 일반 모드
				SetStance(GENERAL_STANCE_TYPE_NEUTRAL);

				// 상태 초기화
				m_accumulatedDeltaX = 0.0f;
				m_accumulatedDeltaY = 0.0f;
				UpdateUISelection(GENERAL_ATTACK_DIR_TYPE_NONE, std::nullopt);

				// 전투 모드 해제 시 카메라 복귀
				if (auto* mainCamera = CameraComponent::GetMainCamera())
				{	
					// 캐릭터 있을 때
					if (auto* owner = GetGameObject())
					{
						mainCamera->SetLookAtTarget(owner->GetComponentHandle<Transform>());
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
				}
			}
		}

		// 2. NEUTRAL에서 공격 처리
		if (m_currentStance == GENERAL_STANCE_TYPE_NEUTRAL)
		{
			// 좌클릭 시 공격 (방향 NONE, 타입 LIGHT)
			if (GLOBAL(InputGlobal).GetInputDown(VK_LBUTTON))
			{
				FB_STRUCTS::GeneralAttackInfo attackInfo(GENERAL_ATTACK_TYPE_LIGHT, GENERAL_ATTACK_DIR_TYPE_NONE);
				auto pb = NetBridge::C2S::Make_CS_PLAYER_ATTACK_PACKET(&attackInfo);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

				if (auto* fsm = GetGameObject()->GetComponent<FSMComponent>())
				{
					fsm->SetCurAttackType(static_cast<uint8_t>(GENERAL_ATTACK_TYPE_LIGHT));
					fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY);
				}
			}
		}
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

		if (m_controlMode == ControlType::Local)
		{
			// Alt 키: 카메라 락온 타겟 변경 요청
			if (GLOBAL(InputGlobal).GetInputDown(VK_MENU))
			{
				auto pb = NetBridge::C2S::Make_CS_CHANGE_CAMERA_TARGET_PACKET(0);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
			}

			// 입력 버퍼링 타이머 갱신
			if (m_pendingLeftClick || m_pendingRightClick)
			{
				m_inputBufferTimer += deltaTime;
			}
			else
			{
				m_inputBufferTimer = 0.0f;
			}

			ProcessMouseInput();
		}

		UpdateUIPosition();
	}
}

void BattleUIControllerComponent::InitStance(GENERAL_STANCE_TYPE stance)
{
	SetStance(stance);
	DEBUG_LOG_FMT("[BattleUI] InitStance Called! Stance: {}\n", static_cast<int>(stance));
}

void BattleUIControllerComponent::SetStance(GENERAL_STANCE_TYPE stance)
{
	m_currentStance = stance;
	ToggleUI(stance == GENERAL_STANCE_TYPE_COMBAT);
}

void BattleUIControllerComponent::OnDestroy()
{
	if (m_uiRootObjHandle.IsValid())
	{
		if (auto* scene = GLOBAL(SceneGlobal).GetActiveScene())
		{
			scene->DestroyGameObject(m_uiRootObjHandle);
		}
		m_uiRootObjHandle = HandleOf<GameObject>::Invalid();
	}

	if (m_controlMode == ControlType::Local)
	{
		GLOBAL(InputGlobal).SetMouseLocked(false);
	}

	m_managedImages.clear();
	m_managedButtons.clear();
}

float BattleUIControllerComponent::NormalizeAngle(float degrees)
{
	while (degrees < 0.0f)
		degrees += 360.0f;
	while (degrees >= 360.0f)
		degrees -= 360.0f;
	return degrees;
}

void BattleUIControllerComponent::CreateAndSetupUI()
{
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();

	// 1. UI 루트 오브젝트 생성 (플레이어의 자식이 아님 - 회전 상속 방지)
	m_uiRootObjHandle = scene->ReserveGameObject("BattleUIRoot_" + std::to_string(owner->GetServerID()), std::nullopt,
		[scene](GameObject* root) {
			scene->CreateComponentWithInit<RectTransformComponent>(root->GetHandle(), [](RectTransformComponent* rect) {
				rect->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
				rect->SetPivot({ 0.5f, 0.5f });
			});
		}
	);

	// 2. 텍스처 로드
	auto& resGlobal = GLOBAL(ResourceGlobal);
	m_normalTexResource = resGlobal.Load<TextureResource>(L"Resource\\Texture\\normal.evtex");
	m_hoverTexResource = resGlobal.Load<TextureResource>(L"Resource\\Texture\\hovering.evtex");
	m_lightAttackTexResource = resGlobal.Load<TextureResource>(L"Resource\\Texture\\select.evtex");
	m_strongAttackTexResource = resGlobal.Load<TextureResource>(L"Resource\\Texture\\strong.evtex");
	m_areaAttackTexResource = resGlobal.Load<TextureResource>(L"Resource\\Texture\\area.evtex");
	m_disarmTexResource = resGlobal.Load<TextureResource>(L"Resource\\Texture\\disarm.evtex");

	// 3. 자식 UI 오브젝트들 생성
	std::vector<std::string> names = { "UpUI", "LeftUI", "RightUI" };
	for (const auto& name : names)
	{
		scene->ReserveGameObject(name, std::nullopt, [this, scene, name](GameObject* child) {
			auto childHandle = child->GetHandle();
			scene->CreateComponentWithInit<RectTransformComponent>(childHandle, [](auto*) {});
			
			if (auto* rootObj = scene->TryGetGameObject(m_uiRootObjHandle)) {
				child->GetTransform().SetParent(rootObj->GetComponentHandle<Transform>());
			}

			auto imgHandle = scene->CreateComponentWithInit<ImageUIComponent>(childHandle, [this](ImageUIComponent* img) {
				img->SetOrder(10);
				// Depth 정렬 목록 등록
				m_managedImages.push_back({img->GetHandle(), 10});

				// 리소스 참조
				img->SetNormalTextureResource(m_normalTexResource);
				img->SetHoverTextureResource(m_hoverTexResource);
				img->SetPressedTextureResource(m_lightAttackTexResource);
				img->SetDisabledTextureResource(m_normalTexResource);

				img->SetNormalColor({0.7f, 0.7f, 0.7f, 1.0f});
				img->SetHoverColor({0.0f, 0.0f, 0.0f, 1.0f});
				img->SetPressedColor({1.0f, 0.0f, 0.0f, 1.0f});
				img->SetDisabledColor({0.5f, 0.5f, 0.5f, 0.5f});
			});

			auto btnHandle = scene->CreateComponentWithInit<ButtonUIComponent>(childHandle, [this, imgHandle](ButtonUIComponent* btn) {
				btn->SetOrder(11);
				// Depth 정렬 목록 등록
				m_managedButtons.push_back({btn->GetHandle(), 11});

				btn->SetTargetImage(imgHandle);
			});

			if (name == "UpUI") { m_upButtonHandle = btnHandle; m_upImageHandle = imgHandle; }
			else if (name == "LeftUI") { m_leftButtonHandle = btnHandle; m_leftImageHandle = imgHandle; }
			else if (name == "RightUI") { m_rightButtonHandle = btnHandle; m_rightImageHandle = imgHandle; }
		});
	}
}

void BattleUIControllerComponent::SetupListener()
{
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;

	// 람다 캡처[=]로 핸들 값 자체를 복사 (메모리 재할당 대응)
	// 핸들 생성된 후에 실행
	// 리소스 참조를 직접 캡처
	auto lightAttackTexRes = m_lightAttackTexResource;
	auto strongAttackTexRes = m_strongAttackTexResource;
	auto areaAttackTexRes = m_areaAttackTexResource;
	auto disarmTexRes = m_disarmTexResource;
	ControlType controlMode = m_controlMode;

	// 캡처할 핸들들 로컬 변수로 복사
	auto upBtn = m_upButtonHandle;
	auto leftBtn = m_leftButtonHandle;
	auto rightBtn = m_rightButtonHandle;
	auto upImg = m_upImageHandle;
	auto leftImg = m_leftImageHandle;
	auto rightImg = m_rightImageHandle;

	AddListener(m_uiRootObjHandle, [=](GENERAL_ATTACK_DIR_TYPE dir, std::optional<GENERAL_ATTACK_TYPE> type) {
		// this 사용하면 안 됨
		// scene 캡처해서 사용
		if (!scene) return;
		
		// 디버깅: 리스너 실행 시점 핸들 확인
		//if (controlMode == ControlType::Local) {
		//	DEBUG_LOG_FMT("[BattleUI Listener] Executing! UpHandle: {}\n", upImg.GetValue());
		//}

		auto* btnStorage = scene->GetStorage<ButtonUIComponent>();
		auto* imgStorage = scene->GetStorage<ImageUIComponent>();

		auto updateBtn = [&](HandleOf<ButtonUIComponent> btnHandle, HandleOf<ImageUIComponent> imgHandle, bool isSelected) {
			// 1. 텍스처 업데이트
			if (ImageUIComponent* img = imgStorage->Get(imgHandle)) {
				// 공격 타입에 따라 Pressed 텍스처 선택 (기본 Light)
				auto pressedTexRes = lightAttackTexRes;
				if (type.has_value()) {
					if (type.value() == GENERAL_ATTACK_TYPE_HEAVY) pressedTexRes = strongAttackTexRes;
					else if (type.value() == GENERAL_ATTACK_TYPE_AREA) pressedTexRes = areaAttackTexRes;
					else if (type.value() == GENERAL_ATTACK_TYPE_DISARM) pressedTexRes = disarmTexRes;
				}
				img->SetPressedTextureResource(pressedTexRes);
			}
			// 2. 버튼 상태 업데이트
			if (ButtonUIComponent* btn = btnStorage->Get(btnHandle)) {
				if (isSelected) {
					// attackType이 값이 있으면 Pressed, 아니면 Hover
					btn->SetState(type.has_value() ? ButtonState::Pressed : ButtonState::Hover);
				}
				else {
					btn->SetState(ButtonState::Normal);
				}
			}
		};

		updateBtn(upBtn, upImg, dir == GENERAL_ATTACK_DIR_TYPE_TOP);
		updateBtn(leftBtn, leftImg, dir == GENERAL_ATTACK_DIR_TYPE_LEFT);
		updateBtn(rightBtn, rightImg, dir == GENERAL_ATTACK_DIR_TYPE_RIGHT);
	});
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

void BattleUIControllerComponent::UpdateUIPosition()
{
	GameObject* owner = GetGameObject();
	Scene*		scene = owner->GetScene();

	if (!owner || !m_uiRootObjHandle.IsValid()) return;
	
	auto* rootObj = scene->TryGetGameObject(m_uiRootObjHandle);
	if (!rootObj)
		return;

	Transform& playerTr = owner->GetTransform();
	// 1. 타겟의 월드 위치 가져오기
	DirectX::XMFLOAT3 vPos = playerTr.GetWorldPosition();
	DirectX::XMVECTOR worldPos = DirectX::XMLoadFloat3(&vPos);

	// 2. 뷰포트 정보 가져오기
	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
	if (!swapChain) return;

	float width = static_cast<float>(swapChain->GetWidth());
	float height = static_cast<float>(swapChain->GetHeight());

	DirectX::XMMATRIX view = CameraComponent::GetMainViewMatrix();
	DirectX::XMMATRIX proj = CameraComponent::GetMainProjectionMatrix();
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

	// 거리 비례 스케일링 계산
	DirectX::XMVECTOR viewPos = DirectX::XMVector3TransformCoord(worldPos, view);
	float distance = DirectX::XMVectorGetZ(viewPos);
	float scale = 1.0f;
	if (distance > 0.1f) 
	{
		// 거리 반비례
		scale = kReferenceDistance / distance;
		scale = std::clamp(scale, kMinScale, kMaxScale);
	}

	// Depth Sorting
	// Max 200m 가정 정밀도 100
	int32_t depthOffset = static_cast<int32_t>((200.0f - std::min(distance, 200.0f)) * 100.0f);
	if (depthOffset < 0) depthOffset = 0;

	auto* imgStorage = scene->GetStorage<ImageUIComponent>();
	auto* btnStorage = scene->GetStorage<ButtonUIComponent>();

	// ImageUI 갱신
	if (imgStorage)
	{
		for (const auto& pair : m_managedImages)
		{
			if (auto* img = imgStorage->Get(pair.first))
			{
				img->SetOrder(pair.second + depthOffset);
			}
		}
	}

	// ButtonUI 갱신
	if (btnStorage)
	{
		for (const auto& pair : m_managedButtons)
		{
			if (auto* btn = btnStorage->Get(pair.first))
			{
				btn->SetOrder(pair.second + depthOffset);
			}
		}
	}

	// 자식 요소들의 상대 위치와 크기 조절
	SetChildUIPositions(scale);

	// 월드 좌표를 스크린 좌표로 투영
	DirectX::XMVECTOR screenPosVec = DirectX::XMVector3Project(worldPos, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, proj, view, world);
	DirectX::XMFLOAT3 screenPos;
	DirectX::XMStoreFloat3(&screenPos, screenPosVec);

	//// 디버깅
	//if (m_controlMode == ControlType::Local && m_currentStance == GENERAL_STANCE_TYPE_COMBAT)
	//{
	//	DEBUG_LOG_FMT("[BattleUI] ScreenPos: ({:.2f}, {:.2f}, {:.2f}), Viewport: {}x{}\n", 
	//		screenPos.x, screenPos.y,screenPos.z, width, height
	//	);
	//}

	// 카메라 뒤에 있거나 화면 가시 영역을 벗어난 경우 처리
	if (screenPos.x < 0.0f || screenPos.x > 1.0f || 
		screenPos.y < 0.0f || screenPos.y > 1.0f || 
		screenPos.z < 0.0f || screenPos.z > 1.0f)
	{
		ToggleUI(false);
	}
	else
	{
		ToggleUI(true);
	}
	// 중심점 갱신 및 BattleUI 오브젝트 자체의 위치 이동
	m_centerX = screenPos.x;
	m_centerY = screenPos.y;

	// 루트 UI 오브젝트 위치 갱신
	if (auto* rootObj = scene->TryGetGameObject(m_uiRootObjHandle)) {
		if (auto* rectTr = rootObj->GetComponent<RectTransformComponent>()) {
			float offsetX = (screenPos.x - 0.5f) * (float)Variable::kDefaultWindowWidth;
			float offsetY = (screenPos.y - 0.5f) * (float)Variable::kDefaultWindowHeight;
			float containerHalfSize = 50.0f * scale; 
			rectTr->SetOffsetMin({offsetX - containerHalfSize, offsetY - containerHalfSize});
			rectTr->SetOffsetMax({offsetX + containerHalfSize, offsetY + containerHalfSize});

			//// 디버깅
			//if (m_controlMode == ControlType::Local && m_currentStance == GENERAL_STANCE_TYPE_COMBAT)
			//{
			//	DEBUG_LOG_FMT("[UI Debug] Root Active: {}, Pos Min: ({:.2f}, {:.2f})\n", 
			//		rootObj->IsActive(), rectTr->GetOffsetMin().x, rectTr->GetOffsetMin().y);
			//}
		}
	}
	else
	{
		if (m_controlMode == ControlType::Local && m_currentStance == GENERAL_STANCE_TYPE_COMBAT)
		{
			DEBUG_LOG_FMT("[UI Debug] Root Object NOT FOUND!\n");
		}
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
	// 유효하지 않은 상태라면 버퍼 초기화
	if (!m_isAttackValid)
	{
		m_pendingLeftClick = false;
		m_pendingRightClick = false;
		m_inputBufferTimer = 0.0f;
	}
	else
	{
		// 1. 입력 감지
		if (isLeftDown) m_pendingLeftClick = true;
		if (isRightDown) m_pendingRightClick = true;

		// 2. 판정
		std::optional<GENERAL_ATTACK_TYPE> confirmedType = std::nullopt;

		// 마우스 휠: 즉시 Disarm
		if (isMiddleDown)
		{
			confirmedType = GENERAL_ATTACK_TYPE_DISARM;
		}
		// Area Attack: 양쪽 모두 Pending (동시 입력 또는 유예 시간 내 입력)
		else if (m_pendingLeftClick && m_pendingRightClick)
		{
			confirmedType = GENERAL_ATTACK_TYPE_AREA;
		}
		// 유예 시간이 지났는데 한쪽만 눌려있다면 해당 공격 확정
		else if (m_inputBufferTimer >= kInputBufferDuration)
		{
			if (m_pendingLeftClick)
				confirmedType = GENERAL_ATTACK_TYPE_LIGHT;
			else if (m_pendingRightClick)
				confirmedType = GENERAL_ATTACK_TYPE_HEAVY;
		}

		// 3. 실행
		if (confirmedType.has_value())
		{
			if (m_currentSelectedDir != GENERAL_ATTACK_DIR_TYPE_NONE)
			{
				GENERAL_ATTACK_TYPE finalType = confirmedType.value();
				OnGuardDirectionConfirmed(m_currentSelectedDir, finalType);

				// 공격 패킷 전송
				FB_STRUCTS::GeneralAttackInfo attackInfo(finalType, m_currentSelectedDir);
				auto pb = NetBridge::C2S::Make_CS_PLAYER_ATTACK_PACKET(&attackInfo);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

				// FSM 상태 전환
				if (auto* fsm = GetGameObject()->GetComponent<FSMComponent>())
				{
					fsm->SetCurAttackType(static_cast<uint8_t>(finalType));
					fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY);
				}

				// 확정 후 즉시 초기화
				m_accumulatedDeltaX = 0.0f;
				m_accumulatedDeltaY = 0.0f;

				// 공격 피드백 설정 (입력 무효화 + 텍스쳐 잔상)
				m_isAttackValid = false;
				m_attackFeedbackTimer = 10.0f / 60.0f;
				m_lastConfirmedAttackType = finalType;
			}
			
			// 처리 완료 후 버퍼 및 타이머 리셋
			m_pendingLeftClick = false;
			m_pendingRightClick = false;
			m_inputBufferTimer = 0.0f;
		}
	}

	// 버튼 리셋 로직
	if (!isLeftPressed && !isRightPressed && !isMiddlePressed)
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

	m_currentSelectedDir = selectedDir;
	m_currentAttackType = attackType;

	// 리스너들에게 상태 변경 알림
	NotifyListeners(selectedDir, attackType);
}

void BattleUIControllerComponent::NotifyListeners(GENERAL_ATTACK_DIR_TYPE dir, std::optional<GENERAL_ATTACK_TYPE> type)
{
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;

	//if (m_controlMode == ControlType::Local) {
	//	DEBUG_LOG_FMT("[BattleUI Notify] Calling! This: {}, Listeners: {}\n", (void*)this, m_listeners.size());
	//}

	for (auto it = m_listeners.begin(); it != m_listeners.end(); )
	{
		// 관찰자(UI 오브젝트 등)가 여전히 살아있는지 핸들 체크
		if (scene->TryGetGameObject(it->observerHandle))
		{
			it->callback(dir, type);
			++it;
		}
		else
		{
			// 죽은 관찰자는 목록에서 제거 (Lazy Removal)
			it = m_listeners.erase(it);
		}
	}
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
	if (auto rootObj = GetGameObject()->GetScene()->TryGetGameObject(m_uiRootObjHandle))
	{
		rootObj->SetActive(isActive);
	}
}

void BattleUIControllerComponent::TriggerAttackRemote(GENERAL_ATTACK_TYPE type, GENERAL_ATTACK_DIR_TYPE dir)
{
	// 원격 공격 피드백
	OnGuardDirectionConfirmed(dir, type);

	// 타이머 설정
	m_isAttackValid = false; // UpdateUISelection에서 텍스쳐 표시할 때 체크함
	m_attackFeedbackTimer = 10.0f / 60.0f;
	m_lastConfirmedAttackType = type;

	// UI 업데이트
	UpdateUISelection(dir, type);
}