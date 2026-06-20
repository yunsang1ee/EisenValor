#include "stdafxClient.h"
#include "StartScene.h"
#include "ButtonUIComponent.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "TextureResource.h"

namespace
{
	struct StartMenuButtonDesc
	{
		const char* name;
		const wchar_t* normalTexture;
		const wchar_t* hoverTexture;
		float centerY;
	};
}

void StartScene::OnRegisterCustomComponents()
{
}

void StartScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[StartScene] Enter Start Scene.\n");

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
		{"StartButton", L"Resource\\Texture\\Scene\\startbutton.evtex", L"Resource\\Texture\\Scene\\startbuttonselect.evtex", 570.0f},
		{"OptionsButton", L"Resource\\Texture\\Scene\\optionsbutton.evtex", L"Resource\\Texture\\Scene\\optionsbuttonselect.evtex", 640.0f},
		{"CreditButton", L"Resource\\Texture\\Scene\\creditsbutton.evtex", L"Resource\\Texture\\Scene\\creditsbuttonselect.evtex", 710.0f},
		{"ExitButton", L"Resource\\Texture\\Scene\\quitbutton.evtex", L"Resource\\Texture\\Scene\\quitbuttonselect.evtex", 780.0f},
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
					[imageHandle](ButtonUIComponent* buttonComponent)
					{
						buttonComponent->SetTargetImage(imageHandle);
					}
				);
			}
		);
	}
}

void StartScene::OnEndImpl()
{
	DEBUG_LOG_FMT("[StartScene] Scene Ended.\n");
}
