#include "stdafxClient.h"
#include "SampleScene.h"

// Component
#include "Component/PlayerControllerComponent.h"
#include "Component/HealthComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "Component/TeamComponent.h"
#include "Component/VitalUIControllerComponent.h"
#include "Component/StaminaComponent.h"
#include "Component/FSM/FSMComponent.h"
#include "Component/FSM/StatePool.h"

// Engine
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "RectTransformComponent.h"

#include "Transform.h"
#include "MeshResource.h"

// Resource
#include "ResourceGlobal.h"
#include "SkinnedMeshResource.h"

#include "MeshLoader.h"

using Vertex = EvAsset::Vertex;

void SampleScene::OnRegisterCustomComponents()
{
	RegisterComponents<
		PlayerControllerComponent, HealthComponent, BattleUIControllerComponent,
		TeamComponent, VitalUIControllerComponent, StaminaComponent, FSMComponent>();
	DEBUG_LOG_FMT("[SampleScene] Custom components registered\n");
}

void SampleScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[SampleScene] OnStart called\n");	

	//.evscene 로딩 함수
	// auto sceneRes = GLOBAL(ResourceGlobal).Load<SceneResource>("Resource/Scenes/MainArea.evscene");
	// if (sceneRes)
	// {
	// 	LoadFromSceneResource(sceneRes);
	// }	CreateSceneObjects();
	
}

void SampleScene::CreateSceneObjects()
{
	DEBUG_LOG_FMT("[SampleScene] Creating scene objects from assets...\n");

	// 1. Ground 생성
	ReserveGameObject(
		"Ground", std::nullopt,
		[](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, -1.0f, 0.0f);
			tr.SetScale(1.0f);

			auto groundRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Ground.evmesh");
			if (groundRes)
			{
				obj->GetScene()->CreateComponentWithInit<MeshComponent>(
					obj->GetHandle(),
					[groundRes](MeshComponent* mesh) {
						mesh->SetMeshResource(groundRes);
					}
				);
			}
		}
	);

	//// 2. Character 생성
	//ReserveGameObject(
	//	"Character", std::nullopt,
	//	[](GameObject* obj)
	//	{
	//		auto& tr = obj->GetTransform();
	//		tr.SetPosition(10.0f, 0.0f, 0.0f);
	//		tr.SetScale(1.0f);

	//		auto res = GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>("Resource/Models/HumanM_Model.evskin");
	//		if (res)
	//		{
	//			obj->GetScene()->CreateComponentWithInit<SkinnedMeshComponent>(
	//				obj->GetHandle(),
	//				[res](SkinnedMeshComponent* mesh) {
	//					mesh->SetMeshResource(res);
	//				}
	//			);
	//			DEBUG_LOG_FMT("[SampleScene] Character assigned: {}\n", res->GetName());
	//		}
	//	}
	//);

	DX::XMFLOAT3 positions[3] = {{-4.0f, 3.0f, 0.0f}, {0.0f, 1.0f, 5.0f}, {4.0f, 3.0f, 0.0f}};
	DX::XMFLOAT3 rotations[3] = {{10.0f, 15.0f, 0.0f}, {-5.0f, 120.0f, 3.0f}, {8.0f, 210.0f, -10.0f}};
	float scales[3] = {4.0f, 1.0f, 4.0f};

	for (int i = 0; i < 3; ++i)
	{
		ReserveGameObject(
			"Cube_" + std::to_string(i), std::nullopt,
			[i, positions, rotations, scales](GameObject* obj)
			{
				auto& tr = obj->GetTransform();
				tr.SetPosition(positions[i].x, positions[i].y, positions[i].z);
				tr.SetRotation(rotations[i].x, rotations[i].y, rotations[i].z);
				tr.SetScale(scales[i]);

				auto cubeRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Cube.evmesh");
				if (cubeRes)
				{
					obj->GetScene()->CreateComponentWithInit<MeshComponent>(
						obj->GetHandle(),
						[cubeRes](MeshComponent* mesh) {
							mesh->SetMeshResource(cubeRes);
						}
					);
				}
			}
		);
	}
}

void SampleScene::OnEndImpl() {}