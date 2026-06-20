#include "stdafxClient.h"
#include "LoginScene.h"
#include "Component/Login/LoginSceneControllerComponent.h"

void LoginScene::OnRegisterCustomComponents() 
{
	RegisterComponent<LoginSceneControllerComponent>();
}

void LoginScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[LoginScene] STARTED! PLEASE PRESS 'R' KEY.\n");

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
