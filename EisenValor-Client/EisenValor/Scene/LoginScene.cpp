#include "stdafxClient.h"
#include "LoginScene.h"
#include "Component/Login/LoginControllerComponent.h"

void LoginScene::OnRegisterCustomComponents() 
{
	RegisterComponent<LoginControllerComponent>();
}

void LoginScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[LoginScene] STARTED! PLEASE PRESS 'R' KEY.\n");

	// Login Controller Component
	ReserveGameObject(
		"LoginController", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<LoginControllerComponent>(
				obj->GetHandle(),
				[](LoginControllerComponent* login)
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
