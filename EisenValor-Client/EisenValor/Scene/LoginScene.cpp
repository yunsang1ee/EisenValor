#include "stdafxClient.h"
#include "LoginScene.h"
#include "Component/Login/LoginSceneControllerComponent.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "TextureResource.h"

void LoginScene::OnRegisterCustomComponents() 
{
	RegisterComponent<LoginSceneControllerComponent>();
}

void LoginScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[LoginScene] STARTED! PLEASE PRESS 'R' KEY.\n");

	ReserveGameObject(
		"LoginSceneBackground", std::nullopt,
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

	// Login Controller Component
	ReserveGameObject(
		"LoginController", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<LoginSceneControllerComponent>(
				obj->GetHandle(),
				[](LoginSceneControllerComponent* login)
				{
				}
			);
		}
	);
}

void LoginScene::OnEndImpl()
{
	DEBUG_LOG_FMT("[LoginScene] Scene Ended.\n");
}
