#include "stdafxClient.h"
#include "RoomScene.h"

#include "Component/Room/RoomSceneControllerComponent.h"

void RoomScene::OnRegisterCustomComponents() {
	RegisterComponent<RoomSceneControllerComponent>();
}

void RoomScene::OnStartImpl() 
{
	DEBUG_LOG_FMT("[RoomScene] Enter Room Scene.\n");

	
	ReserveGameObject(
		"RoomSceneController", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RoomSceneControllerComponent>(
				obj->GetHandle(), [](RoomSceneControllerComponent* login) {}
			);
		}
	);
}

void RoomScene::OnEndImpl() {}
