#include "stdafxClient.h"
#include "SampleScene.h"
#include "Component/PlayerControllerComponent.h"
#include "Component/HealthComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "Component/TeamComponent.h"
#include "Component/VitalUIControllerComponent.h"
#include "Component/StaminaComponent.h"
#include "Transform.h"
#include "RectTransformComponent.h"
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "UI/UITextureGlobal.h"
#include "ResourceGlobal.h"
#include "MeshResource.h"
#include "MeshComponent.h"

void SampleScene::OnRegisterCustomComponents()
{
	RegisterComponents<
		PlayerControllerComponent, HealthComponent, BattleUIControllerComponent, RectTransformComponent,
		ImageUIComponent, ButtonUIComponent, TeamComponent, VitalUIControllerComponent, StaminaComponent>();
	
	DEBUG_LOG_FMT("[SampleScene] Custom components registered\n");
}

void SampleScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[SampleScene] OnStart called\n");

	GLOBAL(ResourceGlobal).LoadRegistry("Resource/AssetRegistry.evreg");

	CreateSceneObjects();
}

void SampleScene::CreateSceneObjects()
{
	DEBUG_LOG_FMT("[SampleScene] Creating scene objects from exported assets...\n");

	ReserveGameObject(
		"Ground", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, -1.0f, 0.0f);
			tr.SetScale(20.0f);

			CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(),
				[](MeshComponent* mesh) 
				{
					auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Plane.evmesh");
					if (nullptr != meshRes)
					{
						mesh->SetMeshResource(meshRes);
					}
				}
			);
		}
	);

	ReserveGameObject(
		"TestSphere", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, 1.0f, 0.0f);
			tr.SetScale(2.0f);

			CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(),
				[](MeshComponent* mesh) 
				{ 
					auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Sphere.evmesh");
					if (nullptr != meshRes)
					{
						mesh->SetMeshResource(meshRes);
					}
				}
			);
		}
	);

	DEBUG_LOG_FMT("[SampleScene] Scene objects created and assets linked\n");
}

void SampleScene::OnEndImpl() 
{
	DEBUG_LOG_FMT("[SampleScene] OnEnd called\n");
}
