#include "stdafxClient.h"
#include "StartScene.h"
#include "ButtonUIComponent.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "AudioGlobal.h"
#include "ResourceGlobal.h"
#include "SceneGlobal.h"
#include "TextureResource.h"

namespace
{
	enum class StartMenuAction
	{
		None,
		Start,
		Quit
	};

	struct StartMenuButtonDesc
	{
		const char* name;
		const wchar_t* normalTexture;
		const wchar_t* hoverTexture;
		float centerY;
		StartMenuAction action;
	};
}

void StartScene::OnRegisterCustomComponents()
{
}

void StartScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[StartScene] Enter Start Scene.\n");
	GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/startscene.wav", AudioBus::BGM, true);

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
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\Scene\\startscene.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({1.0f, 1.0f, 1.0f, 1.0f});
					image->SetOrder(0);
				}
			);
		}
	);

	const StartMenuButtonDesc buttons[] = {
		{"StartButton", L"Resource\\Texture\\Scene\\startbutton.evtex", L"Resource\\Texture\\Scene\\startbuttonselect.evtex", 700.0f, StartMenuAction::Start},
		{"OptionsButton", L"Resource\\Texture\\Scene\\optionsbutton.evtex", L"Resource\\Texture\\Scene\\optionsbuttonselect.evtex", 770.0f, StartMenuAction::None},
		{"CreditButton", L"Resource\\Texture\\Scene\\creditsbutton.evtex", L"Resource\\Texture\\Scene\\creditsbuttonselect.evtex", 840.0f, StartMenuAction::None},
		{"ExitButton", L"Resource\\Texture\\Scene\\quitbutton.evtex", L"Resource\\Texture\\Scene\\quitbuttonselect.evtex", 910.0f, StartMenuAction::Quit},
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
					[imageHandle, button](ButtonUIComponent* buttonComponent)
					{
						buttonComponent->SetTargetImage(imageHandle);
						buttonComponent->SetOnHover(
							[]()
							{
								GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/click.wav", AudioBus::UI);
							}
						);
						buttonComponent->SetOnClick(
							[button]()
							{
								GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/mouseclick.wav", AudioBus::UI);

								switch (button.action)
								{
								case StartMenuAction::Start:
									#ifdef APPLY_LOBBY_SERVER
									GLOBAL(SceneGlobal).LoadScene("LoginScene");
									#else
									GLOBAL(SceneGlobal).LoadScene("WorldScene");
									#endif
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
}

void StartScene::OnEndImpl()
{
	GLOBAL(AudioGlobal).StopBus(AudioBus::BGM);
	DEBUG_LOG_FMT("[StartScene] Scene Ended.\n");
}
