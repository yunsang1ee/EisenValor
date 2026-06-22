#include "stdafxClient.h"
#include "LoadingScene.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "TextureResource.h"

void LoadingScene::OnRegisterCustomComponents() {}

void LoadingScene::OnStartImpl() {
	DEBUG_LOG_FMT("[LoadingScene] Enter Loading Scene.\n");

	ReserveGameObject(
		"LoadingSceneBackground", std::nullopt,
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
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(
						L"Resource\\Texture\\Scene\\loadingscene.evtex");
					image->SetNormalTextureResource(texture);
					image->SetOrder(0);
				}
			);
		}
	);
}

void LoadingScene::OnEndImpl() {}
