#include "stdafxClient.h"
#include "OptionsMenuComponent.h"

#include "AudioGlobal.h"
#include "ButtonUIComponent.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"
#include "GameObject.h"
#include "ImageUIComponent.h"
#include "InputGlobal.h"
#include "PlayerControllerComponent.h"
#include "RectTransformComponent.h"
#include "RenderPass/DxrRenderPass.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "TextUIComponent.h"
#include "Transform.h"
#include "Util/GameConstants.h"

namespace
{
constexpr int32_t kScrimOrder = 200000;
constexpr int32_t kPanelOrder = kScrimOrder + 10;
constexpr int32_t kButtonOrder = kScrimOrder + 20;
constexpr int32_t kTextOrder = kScrimOrder + 21;

void ParentTo(Scene& scene, GameObject& child, HandleOf<GameObject> parentHandle)
{
	if (auto* parent = scene.TryGetGameObject(parentHandle))
	{
		child.GetTransform().SetParent(parent->GetComponentHandle<Transform>());
	}
}

DxrRenderPass* GetDxrRenderPass()
{
	return static_cast<DxrRenderPass*>(GLOBAL(DxRendererGlobal).GetRenderPass("DXR"));
}

OptionsMenuComponent* GetActiveOptionsMenu()
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return nullptr;
	}

	auto* storage = scene->GetStorage<OptionsMenuComponent>();
	if (!storage)
	{
		return nullptr;
	}

	for (auto& menu : storage->GetList())
	{
		if (menu.GetGameObject() && menu.GetGameObject()->IsActiveInHierarchy())
		{
			return &menu;
		}
	}
	return nullptr;
}
} // namespace

void OptionsMenuComponent::OnStart()
{
	auto* owner = GetGameObject();
	if (!owner || !owner->GetScene())
	{
		return;
	}

	auto& scene = *owner->GetScene();
	m_hasGameplayControls = scene.GetStorage<PlayerControllerComponent>() != nullptr;
	CreateMenuUI(scene);
}

void OptionsMenuComponent::OnUpdate(float deltaTime)
{
	(void)deltaTime;
	auto& input = GLOBAL(InputGlobal);
	if (input.GetUnConsumedInputDown(VK_ESCAPE) && (m_escapeOpenEnabled || m_isOpen))
	{
		input.SetConsumed(VK_ESCAPE);
		SetOpen(!m_isOpen);
	}

	if (m_isOpen)
	{
		RefreshLabels();
	}
}

void OptionsMenuComponent::SetOpen(bool open)
{
	if (m_isOpen == open)
	{
		return;
	}

	auto& input = GLOBAL(InputGlobal);
	if (open)
	{
		m_restoreMouseLock = input.IsMouseLocked();
		input.SetMouseLocked(false);
	}
	else if (m_hasGameplayControls)
	{
		input.SetMouseLocked(m_restoreMouseLock);
	}

	m_isOpen = open;
	if (auto* scene = GetGameObject() ? GetGameObject()->GetScene() : nullptr)
	{
		if (auto* root = scene->TryGetGameObject(m_uiRootHandle))
		{
			root->SetActive(open);
		}

		const auto mouseIndex = ToIndex(Action::MouseCapture);
		if (auto* mouseButton = scene->TryGetGameObject(m_actionObjectHandles[mouseIndex]))
		{
			mouseButton->SetActive(open && m_hasGameplayControls);
		}
	}
	UpdateInteractability();

	if (open)
	{
		RefreshLabels();
	}
}

bool OptionsMenuComponent::OpenActive()
{
	if (auto* menu = GetActiveOptionsMenu())
	{
		menu->SetOpen(true);
		return true;
	}
	return false;
}

bool OptionsMenuComponent::IsOpenInActiveScene()
{
	if (auto* menu = GetActiveOptionsMenu())
	{
		return menu->IsOpen();
	}
	return false;
}

void OptionsMenuComponent::CreateMenuUI(Scene& scene)
{
	m_uiRootHandle = scene.ReserveGameObject(
		"OptionsMenuVisualRoot", std::nullopt,
		[this, &scene](GameObject* root)
		{
			const auto rootHandle = root->GetHandle();
			scene.CreateComponentWithInit<RectTransformComponent>(
				rootHandle,
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 0.0f}, {1.0f, 1.0f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({0.0f, 0.0f});
					rect->SetOffsetMax({0.0f, 0.0f});
				}
			);
			auto scrimImage = scene.CreateComponentWithInit<ImageUIComponent>(
				rootHandle,
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.0f, 0.0f, 0.0f, 0.72f});
					image->SetHoverColor({0.0f, 0.0f, 0.0f, 0.72f});
					image->SetPressedColor({0.0f, 0.0f, 0.0f, 0.72f});
					image->SetOrder(kScrimOrder);
				}
			);
			scene.CreateComponentWithInit<ButtonUIComponent>(
				rootHandle,
				[this, scrimImage](ButtonUIComponent* button)
				{
					button->SetTargetImage(scrimImage);
					button->SetOrder(kScrimOrder);
					button->SetInteractable(m_isOpen);
					button->SetOnClick([]() {});
				}
			);

			const auto panelHandle = scene.ReserveGameObject(
				"OptionsMenuPanel", std::nullopt,
				[&scene, rootHandle](GameObject* panel)
				{
					ParentTo(scene, *panel, rootHandle);
					scene.CreateComponentWithInit<RectTransformComponent>(
						panel->GetHandle(),
						[](RectTransformComponent* rect)
						{
							rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
							rect->SetPivot({0.5f, 0.5f});
							rect->SetOffsetMin({-360.0f, -330.0f});
							rect->SetOffsetMax({360.0f, 330.0f});
						}
					);
					scene.CreateComponentWithInit<ImageUIComponent>(
						panel->GetHandle(),
						[](ImageUIComponent* image)
						{
							image->SetNormalColor({0.05f, 0.05f, 0.05f, 0.96f});
							image->SetOrder(kPanelOrder);
						}
					);
				}
			);

			ReserveText(
				scene, panelHandle, "OptionsMenuTitle", -300.0f, -285.0f, 300.0f, -225.0f, L"OPTIONS", 38.0f, kTextOrder
			);
			ReserveText(
				scene, panelHandle, "OptionsMenuHint", -300.0f, -230.0f, 300.0f, -200.0f, L"ESC TO CLOSE", 15.0f,
				kTextOrder
			);

			ReserveButton(
				scene, panelHandle, "OptionsDisplayModeButton", Action::DisplayMode, -290.0f, -185.0f, 290.0f, -125.0f
			);
			ReserveButton(
				scene, panelHandle, "OptionsPathTracingButton", Action::PathTracing, -290.0f, -105.0f, 290.0f, -45.0f
			);
			ReserveButton(scene, panelHandle, "OptionsRestirButton", Action::RestirPT, -290.0f, -25.0f, 290.0f, 35.0f);
			ReserveButton(
				scene, panelHandle, "OptionsEnvironmentButton", Action::Environment, -290.0f, 55.0f, 290.0f, 115.0f
			);
			ReserveButton(
				scene, panelHandle, "OptionsMouseCaptureButton", Action::MouseCapture, -290.0f, 135.0f, 290.0f, 195.0f
			);
			ReserveButton(scene, panelHandle, "OptionsCloseButton", Action::Close, -290.0f, 235.0f, -10.0f, 295.0f);
			ReserveButton(scene, panelHandle, "OptionsQuitButton", Action::Quit, 10.0f, 235.0f, 290.0f, 295.0f, true);

			root->SetActive(m_isOpen);
		}
	);
}

void OptionsMenuComponent::ReserveText(
	Scene&				 scene,
	HandleOf<GameObject> parentHandle,
	const char*			 name,
	float				 minX,
	float				 minY,
	float				 maxX,
	float				 maxY,
	const wchar_t*		 value,
	float				 fontSize,
	int32_t				 order
)
{
	scene.ReserveGameObject(
		name, std::nullopt,
		[&scene, parentHandle, minX, minY, maxX, maxY, value, fontSize, order](GameObject* obj)
		{
			ParentTo(scene, *obj, parentHandle);
			scene.CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[minX, minY, maxX, maxY](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({minX, minY});
					rect->SetOffsetMax({maxX, maxY});
				}
			);
			scene.CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[value, fontSize, order](TextUIComponent* text)
				{
					text->SetText(value);
					text->SetFontSize(fontSize);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
					text->SetOrder(order);
				}
			);
		}
	);
}

void OptionsMenuComponent::ReserveButton(
	Scene&				 scene,
	HandleOf<GameObject> parentHandle,
	const char*			 name,
	Action				 action,
	float				 minX,
	float				 minY,
	float				 maxX,
	float				 maxY,
	bool				 danger
)
{
	const auto objectHandle = scene.ReserveGameObject(
		name, std::nullopt,
		[this, &scene, parentHandle, action, minX, minY, maxX, maxY, danger](GameObject* obj)
		{
			ParentTo(scene, *obj, parentHandle);
			scene.CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[minX, minY, maxX, maxY](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({minX, minY});
					rect->SetOffsetMax({maxX, maxY});
				}
			);
			auto imageHandle = scene.CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[danger](ImageUIComponent* image)
				{
					image->SetNormalColor({0.05f, 0.05f, 0.05f, 0.94f});
					const DirectX::XMFLOAT4 activeColor = danger ? DirectX::XMFLOAT4{0.93f, 0.16f, 0.16f, 1.0f}
																 : DirectX::XMFLOAT4{0.15f, 0.35f, 1.0f, 1.0f};
					image->SetHoverColor(activeColor);
					image->SetPressedColor(activeColor);
					image->SetOrder(kButtonOrder);
				}
			);
			m_actionTextHandles[ToIndex(action)] = scene.CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"");
					text->SetFontSize(21.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
					text->SetOrder(kTextOrder);
				}
			);
			scene.CreateComponentWithInit<ButtonUIComponent>(
				obj->GetHandle(),
				[this, imageHandle, action](ButtonUIComponent* button)
				{
					button->SetTargetImage(imageHandle);
					button->SetOrder(kButtonOrder);
					button->SetInteractable(m_isOpen && (action != Action::MouseCapture || m_hasGameplayControls));
					button->SetOnHover(
						[]()
						{
							GLOBAL(AudioGlobal)
								.Play2D(
									L"Resource/Sounds/click.wav", AudioBus::UI, false, AudioBalance::kUIButtonVolume
								);
						}
					);
					button->SetOnClick(
						[this, action]()
						{
							GLOBAL(AudioGlobal)
								.Play2D(
									L"Resource/Sounds/mouseclick.wav", AudioBus::UI, false,
									AudioBalance::kUIButtonVolume
								);
							ExecuteAction(action);
						}
					);
				}
			);
			if (action == Action::MouseCapture && !m_hasGameplayControls)
			{
				obj->SetActive(false);
			}
		}
	);
	m_actionObjectHandles[ToIndex(action)] = objectHandle;
}

void OptionsMenuComponent::ExecuteAction(Action action)
{
	switch (action)
	{
	case Action::DisplayMode:
		if (auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain())
		{
			if (swapChain->IsFullscreen())
			{
				swapChain->SetFullscreen(false);
			}
			else
			{
				swapChain->ToggleBorderlessFullscreen();
			}
		}
		break;
	case Action::PathTracing:
		if (auto* dxr = GetDxrRenderPass())
		{
			dxr->TogglePathTracing();
		}
		break;
	case Action::RestirPT:
		if (auto* dxr = GetDxrRenderPass())
		{
			dxr->ToggleRestirPT();
		}
		break;
	case Action::Environment:
		if (auto* dxr = GetDxrRenderPass())
		{
			dxr->ToggleDayEnvironment();
		}
		break;
	case Action::MouseCapture:
		m_restoreMouseLock = !m_restoreMouseLock;
		break;
	case Action::Close:
		SetOpen(false);
		break;
	case Action::Quit:
		PostQuitMessage(0);
		break;
	case Action::Count:
		break;
	}

	if (m_isOpen)
	{
		RefreshLabels();
	}
}

void OptionsMenuComponent::RefreshLabels()
{
	if (auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain())
	{
		const bool fullscreen = swapChain->IsFullscreen() || swapChain->IsBorderlessFullscreen();
		SetActionLabel(Action::DisplayMode, fullscreen ? L"DISPLAY MODE  /  FULLSCREEN" : L"DISPLAY MODE  /  WINDOWED");
	}
	else
	{
		SetActionLabel(Action::DisplayMode, L"DISPLAY MODE  /  UNAVAILABLE");
	}

	if (auto* dxr = GetDxrRenderPass())
	{
		SetActionLabel(
			Action::PathTracing, dxr->IsPathTracingEnabled() ? L"PATH TRACING  /  ON" : L"PATH TRACING  /  OFF"
		);
		SetActionLabel(Action::RestirPT, dxr->IsRestirPTEnabled() ? L"RESTIR PT  /  ON" : L"RESTIR PT  /  OFF");
		SetActionLabel(
			Action::Environment, dxr->IsDayEnvironmentEnabled() ? L"ENVIRONMENT  /  DAY" : L"ENVIRONMENT  /  NIGHT"
		);
	}
	else
	{
		SetActionLabel(Action::PathTracing, L"PATH TRACING  /  UNAVAILABLE");
		SetActionLabel(Action::RestirPT, L"RESTIR PT  /  UNAVAILABLE");
		SetActionLabel(Action::Environment, L"ENVIRONMENT  /  UNAVAILABLE");
	}

	SetActionLabel(Action::MouseCapture, m_restoreMouseLock ? L"MOUSE CAPTURE  /  ON" : L"MOUSE CAPTURE  /  OFF");
	SetActionLabel(Action::Close, L"CLOSE");
	SetActionLabel(Action::Quit, L"QUIT GAME");
}

void OptionsMenuComponent::SetActionLabel(Action action, std::wstring value)
{
	auto* scene = GetGameObject() ? GetGameObject()->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	auto* storage = scene->GetStorage<TextUIComponent>();
	if (!storage)
	{
		return;
	}

	if (auto* text = storage->Get(m_actionTextHandles[ToIndex(action)]))
	{
		text->SetText(std::move(value));
	}
}

void OptionsMenuComponent::UpdateInteractability()
{
	auto* scene = GetGameObject() ? GetGameObject()->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	if (auto* root = scene->TryGetGameObject(m_uiRootHandle))
	{
		if (auto* scrimButton = root->GetComponent<ButtonUIComponent>())
		{
			scrimButton->SetInteractable(m_isOpen);
		}
	}

	for (size_t index = 0; index < kActionCount; ++index)
	{
		if (auto* object = scene->TryGetGameObject(m_actionObjectHandles[index]))
		{
			if (auto* button = object->GetComponent<ButtonUIComponent>())
			{
				const bool isMouseCapture = index == ToIndex(Action::MouseCapture);
				button->SetInteractable(m_isOpen && (!isMouseCapture || m_hasGameplayControls));
			}
		}
	}
}
