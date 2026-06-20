#include "stdafxClient.h"
#include "StartScene.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "TextureResource.h"

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
}

void StartScene::OnEndImpl()
{
	DEBUG_LOG_FMT("[StartScene] Scene Ended.\n");
}
