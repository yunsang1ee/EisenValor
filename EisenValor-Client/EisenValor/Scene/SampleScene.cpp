#include "stdafxClient.h"
#include "SampleScene.h"
#include "Scene\SceneComponentData\TorchEmitterSceneComponentData.h"

// Component
#include "Component/PlayerControllerComponent.h"
#include "Component/HealthComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "Component/TeamComponent.h"
#include "Component/VitalUIControllerComponent.h"
#include "Component/StaminaComponent.h"
#include "Component/FSM/FSMComponent.h"
#include "Component/FSM/StatePool.h"
#include "Component/StressTestComponent.h"
#include "Component/SocketComponent.h"
#include "Component/LocalLightComponent.h"

// Engine
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "RectTransformComponent.h"

#include "Transform.h"
#include "MeshResource.h"

// Resource
#include "ResourceGlobal.h"
#include "SceneResource.h"
#include "SkinnedMeshResource.h"

#include "MeshLoader.h"
#include "MeshComponent.h"
#include "MaterialResource.h"
#include "RaytracingCommon.h"

using Vertex = EvAsset::Vertex;

namespace
{
constexpr std::string_view kDefaultMapScenePath = "Resource/Scenes/Map.evscene";
constexpr std::string_view kTorchPreviewSphereMeshPath = "Resource/Models/Sphere.evmesh";
constexpr float				kTorchVisibleEmissionScale = 1.0f;
constexpr float				kTorchSurfaceLightScale = 30.0f;
} // namespace

void SampleScene::OnRegisterCustomComponents()
{
	RegisterComponents<
		PlayerControllerComponent, HealthComponent, BattleUIControllerComponent, TeamComponent,
		VitalUIControllerComponent, StaminaComponent, FSMComponent, StressTestComponent, SocketComponent,
		LocalLightComponent>();
	DEBUG_LOG_FMT("[SampleScene] Custom components registered\n");
}

void SampleScene::OnRegisterCustomSceneComponentDecoders()
{
	RegisterSceneComponentDecoder<TorchEmitterSceneComponentData>(
		[this](const TorchEmitterSceneComponentData& data, const SceneComponentLoadContext& context)
		{
			ReserveGameObject(
				"TorchEmitterPreview", std::nullopt,
				[this, ownerHandle = context.ownerHandle, sourceRadius = data.GetSourceRadius(),
				 visibleIntensity = data.GetIntensity() * kTorchVisibleEmissionScale,
				 lightIntensity = data.GetIntensity() * kTorchSurfaceLightScale, range = data.GetRange(),
				 colorR = data.GetColor()[0], colorG = data.GetColor()[1], colorB = data.GetColor()[2]](GameObject* obj)
				{
					if (auto* ownerObj = TryGetGameObject(ownerHandle))
					{
						obj->GetTransform().SetParent(ownerObj->GetTransform().GetHandle());
					}

					const float diameter = std::max(sourceRadius * 2.0f, 0.15f);
					obj->GetTransform().SetScale(diameter, diameter, diameter);

					CreateComponentWithInit<MeshComponent>(
						obj->GetHandle(),
						[visibleIntensity, colorR, colorG, colorB](MeshComponent* mesh)
						{
							auto meshRes =
								GLOBAL(ResourceGlobal).Load<MeshResource>(kTorchPreviewSphereMeshPath.data());
							if (!meshRes)
							{
								return;
							}

							mesh->SetMeshResource(meshRes, false);

							auto		material = std::make_shared<MaterialResource>();
							const float albedo[4]{colorR, colorG, colorB, 1.0f};
							const float emissiveColor[3]{colorR, colorG, colorB};
							material->SetData(
								EvAsset::ShadingModel::LitPbr, MATERIAL_FLAG_DOUBLE_SIDED, albedo, 1.0f, 0.0f,
								emissiveColor, std::max(visibleIntensity, 0.0f)
							);
							mesh->SetMaterialResource(0, std::move(material));
						}
					);

					CreateComponentWithInit<LocalLightComponent>(
						obj->GetHandle(),
						[lightIntensity, range, sourceRadius, colorR, colorG, colorB](LocalLightComponent* light)
						{
							light->SetColor(DX::XMFLOAT3(colorR, colorG, colorB));
							light->SetIntensity(lightIntensity);
							light->SetRange(range);
							light->SetSourceRadius(sourceRadius);
						}
					);
				}
			);
		}
	);

	DEBUG_LOG_FMT("[SampleScene] Custom scene component decoders registered\n");
}

void SampleScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[SampleScene] OnStart called\n");

	bool loadedScene = false;
	if (auto sceneResource = GLOBAL(ResourceGlobal).Load<SceneResource>(std::filesystem::path(kDefaultMapScenePath)))
	{
		LoadFromSceneResource(sceneResource);
		loadedScene = true;
		DEBUG_LOG_FMT("[SampleScene] Loaded scene resource: {}\n", kDefaultMapScenePath);
	}

	if (!loadedScene)
	{
		CreateSceneObjects();

		// 서버 없이 테스트를 위한 스트레스 테스트 오브젝트 생성
		// ReserveGameObject(
		//	"StressTester", std::nullopt,
		//	[this](GameObject* obj) { CreateComponent<StressTestComponent>(obj->GetHandle()); }
		//);
	}
}

void SampleScene::CreateSceneObjects()
{
	DEBUG_LOG_FMT("[SampleScene] Creating scene objects from exported assets...\n");

	ReserveGameObject(
		"Ground", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, 0.0f, 0.0f);
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

	/*ReserveGameObject(
		"TestSphere", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, 2.0f, 0.0f);
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
	);*/

	DEBUG_LOG_FMT("[SampleScene] Scene objects created and assets linked\n");
}

void SampleScene::OnEndImpl()
{
	DEBUG_LOG_FMT("[SampleScene] OnEnd called\n");
}
