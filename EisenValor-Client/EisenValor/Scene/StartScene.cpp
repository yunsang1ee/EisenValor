#include "stdafxClient.h"
#include "StartScene.h"
#include "ButtonUIComponent.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "AudioGlobal.h"
#include "Component/OptionsMenuComponent.h"
#include "NetworkGlobal.h"
#include "ResourceGlobal.h"
#include "SceneGlobal.h"
#include "TextureResource.h"
#include "TextUIComponent.h"
#include "Util/GameConstants.h"
#include "Util/SceneNavigation.h"

namespace
{
enum class StartMenuAction
{
	None,
	Start,
	Options,
	Quit
};

struct StartMenuButtonDesc
{
	const char*		name;
	const wchar_t*	normalTexture;
	const wchar_t*	hoverTexture;
	float			centerY;
	StartMenuAction action;
};
} // namespace

void StartScene::OnRegisterCustomComponents()
{
	RegisterComponent<OptionsMenuComponent>();
}

void StartScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[StartScene] Enter Start Scene.\n");
	GLOBAL(AudioGlobal)
		.Play2D(L"Resource/Sounds/startscene.wav", AudioBus::BGM, true, AudioBalance::kFrontEndBgmVolume);

	ReserveGameObject(
		"OptionsMenuController", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<OptionsMenuComponent>(
				obj->GetHandle(), [](OptionsMenuComponent* menu) { menu->SetEscapeOpenEnabled(false); }
			);
		}
	);

	ReserveGameObject(
		"StartSceneBackground", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 0.0f}, {1.0f, 1.0f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({0.0f, 0.0f});
					rect->SetOffsetMax({0.0f, 0.0f});
				}
			);

			CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture =
						GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\Scene\\startscene.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({1.0f, 1.0f, 1.0f, 1.0f});
					image->SetOrder(0);
				}
			);
		}
	);

	const StartMenuButtonDesc buttons[] = {
		{"StartButton", L"Resource\\Texture\\Scene\\startbutton.evtex",
		 L"Resource\\Texture\\Scene\\startbuttonselect.evtex", 700.0f, StartMenuAction::Start},
		{"OptionsButton", L"Resource\\Texture\\Scene\\optionsbutton.evtex",
		 L"Resource\\Texture\\Scene\\optionsbuttonselect.evtex", 770.0f, StartMenuAction::Options},
		{"CreditButton", L"Resource\\Texture\\Scene\\creditsbutton.evtex",
		 L"Resource\\Texture\\Scene\\creditsbuttonselect.evtex", 840.0f, StartMenuAction::None},
		{"ExitButton", L"Resource\\Texture\\Scene\\quitbutton.evtex",
		 L"Resource\\Texture\\Scene\\quitbuttonselect.evtex", 910.0f, StartMenuAction::Quit},
	};

	for (const auto& button : buttons)
	{
		ReserveGameObject(
			button.name, std::nullopt,
			[this, button](GameObject* obj)
			{
				CreateComponentWithInit<RectTransformComponent>(
					obj->GetHandle(),
					[button](RectTransformComponent* rect)
					{
						rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
						rect->SetPivot({0.5f, 0.5f});
						rect->SetOffsetMin({-150.0f, button.centerY - 30.0f});
						rect->SetOffsetMax({150.0f, button.centerY + 30.0f});
					}
				);

				auto imageHandle = CreateComponentWithInit<ImageUIComponent>(
					obj->GetHandle(),
					[button](ImageUIComponent* image)
					{
						auto normalTexture = GLOBAL(ResourceGlobal).Load<TextureResource>(button.normalTexture);
						auto hoverTexture = GLOBAL(ResourceGlobal).Load<TextureResource>(button.hoverTexture);
						image->SetNormalTextureResource(normalTexture);
						image->SetHoverTextureResource(hoverTexture);
						image->SetPressedTextureResource(hoverTexture);
						image->SetOrder(10);
					}
				);

				CreateComponentWithInit<ButtonUIComponent>(
					obj->GetHandle(),
					[this, imageHandle, button](ButtonUIComponent* buttonComponent)
					{
						buttonComponent->SetTargetImage(imageHandle);
						buttonComponent->SetOnHover(
							[]()
							{
								GLOBAL(AudioGlobal)
									.Play2D(
										L"Resource/Sounds/click.wav", AudioBus::UI, false, AudioBalance::kUIButtonVolume
									);
							}
						);
						buttonComponent->SetOnClick(
							[this, button]()
							{
								GLOBAL(AudioGlobal)
									.Play2D(
										L"Resource/Sounds/mouseclick.wav", AudioBus::UI, false,
										AudioBalance::kUIButtonVolume
									);

								switch (button.action)
								{
								case StartMenuAction::Start:
#ifdef APPLY_LOBBY_SERVER
									if (!GLOBAL(NetBridge::NetworkGlobal).Init(m_lobbyAddress, G_LOBBY_SERVER_PORT))
									{
										MessageBoxW(GetActiveWindow(),
											L"Cannot connect to the lobby server. Try again, or open PATH TRACING DEMO.",
											L"Connection Failed", MB_OK | MB_ICONERROR);
										break;
									}
									SceneNavigation::Request("LoginScene");
#else
									SceneNavigation::Request("WorldScene");
#endif
									break;
								case StartMenuAction::Options:
									OptionsMenuComponent::OpenActive();
									break;
								case StartMenuAction::Quit:
									PostQuitMessage(0);
									break;
								case StartMenuAction::None:
									break;
								}
							}
						);
					}
				);
			}
		);
	}

	ReserveGameObject(
		"PathTracingDemoButton", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 1.0f}, {0.0f, 1.0f});
					rect->SetOffsetMin({40.0f, -100.0f});
					rect->SetOffsetMax({400.0f, -40.0f});
				}
			);
			const auto image = CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* component)
				{
					component->SetNormalColor({0.05f, 0.05f, 0.05f, 0.94f});
					component->SetHoverColor({0.15f, 0.35f, 1.0f, 1.0f});
					component->SetPressedColor({0.15f, 0.35f, 1.0f, 1.0f});
					component->SetOrder(10);
				}
			);
			CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"PATH TRACING DEMO");
					text->SetFontSize(24.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1, 1, 1, 1});
					text->SetOrder(11);
				}
			);
			CreateComponentWithInit<ButtonUIComponent>(
				obj->GetHandle(),
				[image](ButtonUIComponent* button)
				{
					button->SetTargetImage(image);
					button->SetOrder(10);
					button->SetOnClick(
						[]()
						{
							if (!OptionsMenuComponent::IsOpenInActiveScene())
							{
								SceneNavigation::Request("PathTracingDemoScene");
							}
						});
				}
			);
		}
	);
}

void StartScene::OnEndImpl()
{
	DEBUG_LOG_FMT("[StartScene] Scene Ended.\n");
}
