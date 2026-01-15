#include "stdafxClient.h"
#include "SampleScene.h"
#include "Component/PlayerController.h"
#include "Component/HealthComponent.h"


void SampleScene::OnRegisterCustomComponents()
{
	RegisterComponents<PlayerController, HealthComponent>();
	DEBUG_LOG_FMT("[SampleScene] Custom components registered\n");
}

void SampleScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[SampleScene] OnStart called\n");
	CreateSceneObjects();
}

void SampleScene::CreateSceneObjects() {}

void SampleScene::OnEndImpl() {}
