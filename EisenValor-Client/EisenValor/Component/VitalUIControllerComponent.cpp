#include "stdafxClient.h"
#include "InputGlobal.h"
#include "VitalUIControllerComponent.h"
#include "HealthComponent.h"
#include "StaminaComponent.h"
#include "TeamComponent.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"
#include "SceneGlobal.h"
#include <Scene.h>
#include <GameObject.h>
#include <Transform.h>
#include <Packets/Enums_generated.h>
#include <UI/UITextureGlobal.h>
#include <algorithm>
#include <functional>

void VitalUIControllerComponent::OnStart()
{
	// 이미 초기화된 경우 중복 실행 방지
	if (m_rootUI.IsValid() && GetGameObject()->GetScene()->TryGetGameObject(m_rootUI))
	{
		return;
	}

	GameObject* owner = GetGameObject();
	if (owner)
	{
		// Stamina Component가 붙어있으면 플레이어
		m_isPlayer = owner->HasComponent<StaminaComponent>();
	}

	// UI 동적 생성 및 초기화
	CreateAndSetupUI();
}

void VitalUIControllerComponent::OnUpdate(float deltaTime)
{
	if (!m_rootUI.IsValid()) return;
	
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;

	// 1. HP Bar
	if (auto* health = owner->GetComponent<HealthComponent>()) 
	{
		uint32_t currentHP = health->GetHealth();
		uint32_t maxHP = health->GetMaxHealth();

		// 디버깅
		if (GLOBAL(InputGlobal).GetInputDown(VK_F2))
		{
			DEBUG_LOG_FMT("[HP Debug] Object ID: {}, HP: {} / {}\n", owner->GetServerID(), currentHP, maxHP);
		}

		float ratio = static_cast<float>(currentHP) / maxHP;
		ratio = std::clamp(ratio, 0.0f, 1.0f);

		if (m_hpFill.IsValid())
		{
			if (auto* img = scene->GetStorage<ImageUIComponent>()->Get(m_hpFill))
			{
				if (auto* rect = img->GetGameObject()->GetComponent<RectTransformComponent>()) 
				{
					rect->SetAnchors({0.0f, 0.0f}, {ratio, 1.0f});
				}
			}
		}
	}

	// 2. Stamina Bar
	if (m_isPlayer)
	{
		if (auto* stamina = owner->GetComponent<StaminaComponent>()) 
		{
			float ratio = stamina->GetStaminaRatio();
			ratio = std::clamp(ratio, 0.0f, 1.0f);

			if (m_staminaFill.IsValid())
			{
				if (auto* img = scene->GetStorage<ImageUIComponent>()->Get(m_staminaFill))
				{
					if (auto* rect = img->GetGameObject()->GetComponent<RectTransformComponent>()) 
					{
						rect->SetAnchors({0.0f, 0.0f}, {ratio, 1.0f});
					}
				}
			}
		}
	}

	// 3. 위치 동기화 (Billboard)
	auto* rootObj = scene->TryGetGameObject(m_rootUI);
	if (!rootObj) return;

	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
	if (!swapChain) return;

	float screenW = static_cast<float>(swapChain->GetWidth());
	float screenH = static_cast<float>(swapChain->GetHeight());

	DirectX::XMMATRIX view = CameraComponent::GetMainViewMatrix();
	DirectX::XMMATRIX proj = CameraComponent::GetMainProjectionMatrix();
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

	DirectX::XMFLOAT3 worldPos = owner->GetTransform().GetWorldPosition();
	worldPos.y += 1.2f;

	DirectX::XMVECTOR worldPosVec = DirectX::XMLoadFloat3(&worldPos);

	// 거리 비례 스케일링
	DirectX::XMVECTOR viewPos = DirectX::XMVector3TransformCoord(worldPosVec, view);
	float distance = DirectX::XMVectorGetZ(viewPos);
	float scale = 1.0f;
	const float kReferenceDistance = 10.0f;
	const float kMinScale = 0.3f;
	const float kMaxScale = 1.5f;

	if (distance > 0.1f)
	{
		scale = kReferenceDistance / distance;
		scale = std::clamp(scale, kMinScale, kMaxScale);
	}

	// Depth Sorting
	// Max 200m 가정 정밀도 100
	int32_t depthOffset = static_cast<int32_t>((200.0f - std::min(distance, 200.0f)) * 100.0f);
	if (depthOffset < 0) depthOffset = 0;

	auto* imgStorage = scene->GetStorage<ImageUIComponent>();
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

	DirectX::XMVECTOR screenPosVec = DirectX::XMVector3Project(worldPosVec, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, proj, view, world);

	DirectX::XMFLOAT3 screenPos;
	DirectX::XMStoreFloat3(&screenPos, screenPosVec);

	// 카메라 뒤에 있거나 화면 가시 영역을 벗어난 경우 처리
	if (screenPos.x < 0.0f || screenPos.x > 1.0f || 
		screenPos.y < 0.05f || screenPos.y > 1.0f || 
		screenPos.z < 0.0f || screenPos.z > 1.0f)
	{
		ToggleUI(false);
	}
	else
	{
		ToggleUI(true);

		if (auto* rect = rootObj->GetComponent<RectTransformComponent>()) 
		{
			// 수동 스케일링
			SetChildUIPositions(scale);

			float finalX = (screenPos.x - 0.5f) * (float)Variable::kDefaultWindowWidth;
			float finalY = (screenPos.y - 0.5f) * (float)Variable::kDefaultWindowHeight;
			
			// 크기 0
			rect->SetOffsetMin({ finalX, finalY });
			rect->SetOffsetMax({ finalX, finalY });
		}
	}
}

void VitalUIControllerComponent::SetChildUIPositions(float scale)
{
	auto scene = GetGameObject()->GetScene();
	if (!scene) return;

	// 1. 깃발 아이콘 스케일링
	if (auto flagObj = scene->TryGetGameObject(m_flagRootHandle))
	{
		if (auto rect = flagObj->GetComponent<RectTransformComponent>())
		{
			float sSize = kFlagSize * scale;

			// 위치 조절용 변수
			float flagMarginX = -25.0f;
			float flagOffsetY = 5.0f;

			rect->SetSizeDelta({sSize, sSize});
			rect->SetOffsetMin({(-kFlagSize + flagMarginX) * scale, (-kFlagSize * 0.5f + flagOffsetY) * scale});
			rect->SetOffsetMax({(flagMarginX + flagOffsetY) * scale, (kFlagSize * 0.5f + flagOffsetY) * scale});
		}
	}

	// 이동
	float moveX = -40.0f;

	// 2. HP Bar 스케일링
	if (auto backObj = scene->TryGetGameObject(m_hpRootHandle))
	{
		if (auto rect = backObj->GetComponent<RectTransformComponent>())
		{
			float sw = kHPBarWidth * scale;
			float sh = kHPBarHeight * scale;
			float yPos = -(sh * 0.5f + kPadding * scale * 0.5f);
			rect->SetSizeDelta({sw, sh});

			// HP Bar
			rect->SetOffsetMin({moveX * scale, yPos});
			rect->SetOffsetMax({(kHPBarWidth + moveX) * scale, yPos + sh});
		}
	}

	// 3. Stamina Bar 스케일링 (Player Only)
	if (m_isPlayer)
	{
		if (auto backObj = scene->TryGetGameObject(m_staminaRootHandle))
		{
			if (auto rect = backObj->GetComponent<RectTransformComponent>())
			{
				float sw = kStaminaBarWidth * scale;
				float sh = kStaminaBarHeight * scale;
				float yPos = (sh * 0.5f + kPadding * scale * 0.5f);
				rect->SetSizeDelta({sw, sh});

				// Stamina Bar
				rect->SetOffsetMin({moveX * scale, yPos});
				rect->SetOffsetMax({(kStaminaBarWidth + moveX) * scale, yPos + sh});
			}
		}
	}
}

void VitalUIControllerComponent::ToggleUI(bool isActive)
{
	if (auto rootObj = GetGameObject()->GetScene()->TryGetGameObject(m_rootUI))
	{
		rootObj->SetActive(isActive);
	}
}

void VitalUIControllerComponent::OnDestroy()
{
	if (m_rootUI.IsValid())
	{
		if (auto* scene = GLOBAL(SceneGlobal).GetActiveScene())
		{
			scene->DestroyGameObject(m_rootUI);
		}
		m_rootUI = HandleOf<GameObject>::Invalid();
	}
	
	// 관리 목록 초기화
	m_managedImages.clear();
}

void VitalUIControllerComponent::CreateAndSetupUI()
{
	GameObject* owner = GetGameObject();
	if (!owner) return;
	Scene* scene = owner->GetScene();
	if (!scene) return;
	
	auto& texGlobal = UITextureGlobal::GetInstance();

	// 1. UI 루트 오브젝트 생성
	std::string rootName = "VitalUIRoot_" + std::to_string(owner->GetServerID());

	m_rootUI = scene->ReserveGameObject(rootName, std::nullopt,
		[scene](GameObject* root) {
			scene->CreateComponentWithInit<RectTransformComponent>(root->GetHandle(), [](RectTransformComponent* rect) {
				rect->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
				rect->SetPivot({ 0.5f, 0.5f });
			});
		}
	);
	
	// 2. 깃발 아이콘 생성
	m_flagRootHandle = scene->ReserveGameObject("FlagIcon", std::nullopt, [this, scene, &texGlobal](GameObject* flagObj) {
		auto flagHandle = flagObj->GetHandle();
		
		if (auto rootObj = scene->TryGetGameObject(m_rootUI)) {
			flagObj->GetTransform().SetParent(rootObj->GetComponentHandle<Transform>());
		}

		scene->CreateComponentWithInit<RectTransformComponent>(flagHandle, [](RectTransformComponent* rect) {
			rect->SetPivot({ 0.5f, 0.5f });
			rect->SetAnchors({ 0.0f, 0.5f }, { 0.0f, 0.5f });
		});

		m_flagIcon = scene->CreateComponentWithInit<ImageUIComponent>(flagHandle, [this, &texGlobal](ImageUIComponent* img) {
			img->SetOrder(12);
			// 관리 목록 등록
			m_managedImages.push_back({img->GetHandle(), 12});

			// TeamComponent
			auto teamComp = GetGameObject()->GetComponent<TeamComponent>();
			FB_ENUMS::TEAM_TYPE team = teamComp ? teamComp->GetTeamType() : FB_ENUMS::TEAM_TYPE_OFFENSE;

			uint32_t texId = (team == FB_ENUMS::TEAM_TYPE_OFFENSE) ? 
				texGlobal.LoadTexture(L"Resource\\Texture\\FlagBlue.dds") : 
				texGlobal.LoadTexture(L"Resource\\Texture\\FlagRed.dds");
			img->SetNormalTexture(texId);
		});
	});

	// 3. HP Bar 세트 생성
	m_hpRootHandle = scene->ReserveGameObject("HP_Back", std::nullopt, [this, scene, &texGlobal](GameObject* backObj) {
		auto backHandle = backObj->GetHandle();

		if (auto rootObj = scene->TryGetGameObject(m_rootUI)) {
			backObj->GetTransform().SetParent(rootObj->GetComponentHandle<Transform>());
		}

		scene->CreateComponentWithInit<RectTransformComponent>(backHandle, [](RectTransformComponent* rect) {
			rect->SetPivot({ 0.0f, 0.5f });
			rect->SetAnchors({ 0.0f, 0.5f }, { 0.0f, 0.5f });
		});

		scene->CreateComponentWithInit<ImageUIComponent>(backHandle, [this, &texGlobal](ImageUIComponent* img) {
			img->SetOrder(11);
			// 관리 목록 등록 (Back)
			m_managedImages.push_back({img->GetHandle(), 11});
			img->SetNormalTexture(texGlobal.LoadTexture(L"Resource\\Texture\\HPback.dds"));
		});

		// HP Fill
		scene->ReserveGameObject("HP_Fill", std::nullopt, [this, scene, backHandle, &texGlobal](GameObject* fillObj) {
			auto fillHandle = fillObj->GetHandle();
			if (auto backObj = scene->TryGetGameObject(backHandle)) {
				fillObj->GetTransform().SetParent(backObj->GetComponentHandle<Transform>());
			}

			scene->CreateComponentWithInit<RectTransformComponent>(fillHandle, [](RectTransformComponent* rect) {
				rect->SetPivot({ 0.0f, 0.5f });
				rect->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
				rect->SetOffsetMin({ kPadding, kPadding });
				rect->SetOffsetMax({ -kPadding, -kPadding });
			});

			m_hpFill = scene->CreateComponentWithInit<ImageUIComponent>(fillHandle, [this, &texGlobal](ImageUIComponent* img) {
				img->SetOrder(11);
				// 관리 목록 등록 (Fill)
				m_managedImages.push_back({img->GetHandle(), 11});
				img->SetNormalTexture(texGlobal.LoadTexture(L"Resource\\Texture\\HPfill.dds"));
				img->SetNormalColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 하얀색
			});
		});
	});

	// 4. Stamina Bar 세트 생성 (Player 전용)
	if (m_isPlayer)
	{
		m_staminaRootHandle = scene->ReserveGameObject("Stamina_Back", std::nullopt, [this, scene, &texGlobal](GameObject* backObj) {
			auto backHandle = backObj->GetHandle();

			if (auto rootObj = scene->TryGetGameObject(m_rootUI)) {
				backObj->GetTransform().SetParent(rootObj->GetComponentHandle<Transform>());
			}

			scene->CreateComponentWithInit<RectTransformComponent>(backHandle, [](RectTransformComponent* rect) {
				rect->SetPivot({ 0.0f, 0.5f });
				rect->SetAnchors({ 0.0f, 0.5f }, { 0.0f, 0.5f });
			});

			scene->CreateComponentWithInit<ImageUIComponent>(backHandle, [this, &texGlobal](ImageUIComponent* img) {
				img->SetOrder(10);
				// 관리 목록 등록 (Back)
				m_managedImages.push_back({img->GetHandle(), 10});
				img->SetNormalTexture(texGlobal.LoadTexture(L"Resource\\Texture\\Staminaback.dds"));
			});

			// Stamina Fill
			scene->ReserveGameObject("Stamina_Fill", std::nullopt, [this, scene, backHandle, &texGlobal](GameObject* fillObj) {
				auto fillHandle = fillObj->GetHandle();
				if (auto backObj = scene->TryGetGameObject(backHandle)) {
					fillObj->GetTransform().SetParent(backObj->GetComponentHandle<Transform>());
				}

				scene->CreateComponentWithInit<RectTransformComponent>(fillHandle, [](RectTransformComponent* rect) {
					rect->SetPivot({ 0.0f, 0.5f });
					rect->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
					rect->SetOffsetMin({ kPadding, kPadding });
					rect->SetOffsetMax({ -kPadding, -kPadding });
				});

				m_staminaFill = scene->CreateComponentWithInit<ImageUIComponent>(fillHandle, [this, &texGlobal](ImageUIComponent* img) {
					img->SetOrder(10);
					// 관리 목록 등록 (Fill)
					m_managedImages.push_back({img->GetHandle(), 10});
					img->SetNormalTexture(texGlobal.LoadTexture(L"Resource\\Texture\\Staminafill.dds"));
					img->SetNormalColor({ 0.0f, 1.0f, 1.0f, 1.0f }); // 민트색
				});
			});
		});
	}
}