#include "stdafxClient.h"
#include "LobbyScene.h"

#include "Component/Lobby/LobbySceneControllerComponent.h"

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
}

void LobbyScene::OnEndImpl() {}
