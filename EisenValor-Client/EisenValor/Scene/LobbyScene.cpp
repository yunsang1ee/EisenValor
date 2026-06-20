#include "stdafxClient.h"
#include "LobbyScene.h"

#include "Component/Lobby/LobbySceneControllerComponent.h"
#include "RectTransformComponent.h"
#include "TextUIComponent.h"

void LobbyScene::OnRegisterCustomComponents() 
{
	RegisterComponent<LobbySceneControllerComponent>();
}

void LobbyScene::OnStartImpl() {

	DEBUG_LOG_FMT("[LobbyScene] Enter Lobby Scene.\n");

	ReserveGameObject(
		"LobbySceneController", std::nullopt, [this](GameObject* obj)
		{
			CreateComponentWithInit<LobbySceneControllerComponent>(
				obj->GetHandle(), [](LobbySceneControllerComponent* login) {}
			);
		}
	);

	ReserveGameObject(
		"LobbyTextDebug", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 0.0f}, {0.0f, 0.0f});
					rect->SetPivot({0.0f, 0.0f});
					rect->SetOffsetMin({40.0f, 40.0f});
					rect->SetOffsetMax({520.0f, 96.0f});
				}
			);

			CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"로비 서버 연결됨");
					text->SetFontSize(32.0f);
					text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
					text->SetOrder(100);
				}
			);
		}
	);
}

void LobbyScene::OnEndImpl() {}
