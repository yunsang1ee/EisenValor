#include "stdafxClient.h"
#include "WorldScene.h"
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
#include "Component/AttackRangeDebugComponent.h"
#include "Component/FootIKComponent.h"
#include "Component/World/WorldSceneControllerComponent.h"

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

#include "MeshComponent.h"
#include "MaterialResource.h"
#include "RaytracingCommon.h"

#include <array>

using Vertex = EvAsset::Vertex;

namespace
{
constexpr std::string_view kDefaultMapScenePath = "Resource/Scenes/Map.evscene";
constexpr std::string_view kTorchPreviewSphereMeshPath = "Resource/Models/Sphere.evmesh";
constexpr std::string_view kDebugNavMeshMeshPath = "Resource/Models/Debug/NavMesh.evmesh";
constexpr std::string_view kDebugMapObjMeshPath = "Resource/Models/Debug/MapObj.evmesh";
constexpr bool			   kEnableDebugNavMeshPreview = true;
constexpr bool			   kEnableDebugMapObjPreview = true;
constexpr float			   kDebugNavMeshYOffset = 0.0f;
constexpr float			   kDebugMapObjYOffset = 0.0f;
constexpr float			   kTorchPreviewSphereScale = 1.5f;
constexpr float			   kTorchPreviewSphereMinDiameter = 0.15f;
constexpr float			   kTorchTransportEmissionScale = 65.0f;
constexpr float			   kTorchVisibleEmissionScale = 2.5f;

struct TorchPreviewColor
{
	float r;
	float g;
	float b;
};

TorchPreviewColor GetTorchPreviewColor(uint32_t nodeIndex)
{
	static constexpr std::array<TorchPreviewColor, 8> kPalette = {{
		{1.00f, 0.17f, 0.18f},
		{1.00f, 0.52f, 0.12f},
		{1.00f, 0.88f, 0.20f},
		{0.25f, 1.00f, 0.35f},
		{0.15f, 0.95f, 0.85f},
		{0.18f, 0.58f, 1.00f},
		{0.48f, 0.25f, 1.00f},
		{1.00f, 0.20f, 0.72f},
	}};

	uint32_t hash = nodeIndex + 0x9e3779b9u;
	hash ^= hash >> 16u;
	hash *= 0x7feb352du;
	hash ^= hash >> 15u;
	hash *= 0x846ca68bu;
	hash ^= hash >> 16u;
	return kPalette[hash % kPalette.size()];
}
} // namespace

void WorldScene::OnRegisterCustomComponents()
{
	RegisterComponents<
		PlayerControllerComponent, HealthComponent, BattleUIControllerComponent, TeamComponent,
		VitalUIControllerComponent, StaminaComponent, FSMComponent, StressTestComponent, SocketComponent,
		AttackRangeDebugComponent, WorldSceneControllerComponent, FootIKComponent>();
	DEBUG_LOG_FMT("[WorldScene] Custom components registered\n");
}

void WorldScene::OnRegisterCustomSceneComponentDecoders()
{
	RegisterSceneComponentDecoder<TorchEmitterSceneComponentData>(
		[this](const TorchEmitterSceneComponentData& data, const SceneComponentLoadContext& context)
		{
			const TorchPreviewColor previewColor = GetTorchPreviewColor(context.nodeIndex);
			ReserveGameObject(
				"TorchEmitterPreview", std::nullopt,
				[this, ownerHandle = context.ownerHandle, sourceRadius = data.GetSourceRadius(),
				 transportIntensity = data.GetIntensity() * kTorchTransportEmissionScale,
				 visibleIntensity = data.GetIntensity() * kTorchVisibleEmissionScale, colorR = previewColor.r,
				 colorG = previewColor.g, colorB = previewColor.b](GameObject* obj)
				{
					if (auto* ownerObj = TryGetGameObject(ownerHandle))
					{
						obj->GetTransform().SetParent(ownerObj->GetTransform().GetHandle());
					}

					const float diameter =
						std::max(sourceRadius * 2.0f * kTorchPreviewSphereScale, kTorchPreviewSphereMinDiameter);
					obj->GetTransform().SetScale(diameter, diameter, diameter);

					CreateComponentWithInit<MeshComponent>(
						obj->GetHandle(),
						[transportIntensity, visibleIntensity, colorR, colorG, colorB](MeshComponent* mesh)
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
							const float visibleEmissiveColor[3]{colorR, colorG, colorB};
							material->SetData(
								EvAsset::ShadingModel::LitPbr, MATERIAL_FLAG_DOUBLE_SIDED, albedo, 1.0f, 0.0f,
								emissiveColor, std::max(transportIntensity, 0.0f), visibleEmissiveColor,
								std::max(visibleIntensity, 0.0f)
							);
							mesh->SetMaterialResource(0, std::move(material));
						}
					);
				}
			);
		}
	);

	DEBUG_LOG_FMT("[WorldScene] Custom scene component decoders registered\n");
}

void WorldScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[WorldScene] OnStart called\n");

	bool loadedScene = false;
	if (auto sceneResource = GLOBAL(ResourceGlobal).Load<SceneResource>(std::filesystem::path(kDefaultMapScenePath)))
	{
		LoadFromSceneResource(sceneResource);
		loadedScene = true;
		DEBUG_LOG_FMT("[WorldScene] Loaded scene resource: {}\n", kDefaultMapScenePath);
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

	ReserveGameObject(
		"WorldSceneController", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<WorldSceneControllerComponent>(
				obj->GetHandle(), [](WorldSceneControllerComponent* login) {}
			);
		}
	);
}

void WorldScene::CreateSceneObjects()
{
	DEBUG_LOG_FMT("[WorldScene] Creating scene objects from exported assets...\n");

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

	DEBUG_LOG_FMT("[WorldScene] Scene objects created and assets linked\n");
}

void WorldScene::OnEndImpl()
{
	DEBUG_LOG_FMT("[WorldScene] OnEnd called\n");
}

void WorldScene::CreateDebugNavMeshPreview()
{
	if (!kEnableDebugNavMeshPreview)
	{
		return;
	}

	ReserveGameObject(
		"Debug_NavMesh", std::nullopt,
		[this](GameObject* obj)
		{
			obj->GetTransform().SetPosition(0.0f, kDebugNavMeshYOffset, 0.0f);

			CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(),
				[](MeshComponent* mesh)
				{
					auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>(kDebugNavMeshMeshPath.data());
					if (!meshRes)
					{
						DEBUG_LOG_FMT("[WorldScene] Debug NavMesh mesh not found: {}\n", kDebugNavMeshMeshPath);
						return;
					}

					mesh->SetMeshResource(meshRes, false);

					auto		material = std::make_shared<MaterialResource>();
					const float albedo[4]{0.05f, 0.85f, 0.45f, 1.0f};
					const float emissiveColor[3]{0.05f, 0.85f, 0.45f};
					material->SetData(
						EvAsset::ShadingModel::LitPbr, MATERIAL_FLAG_DOUBLE_SIDED, albedo, 1.0f, 0.0f, emissiveColor,
						1.5f, emissiveColor, 1.5f
					);
					mesh->SetMaterialResource(0, std::move(material));
				}
			);
		}
	);
}

void WorldScene::CreateDebugMapObjPreview()
{
	if (!kEnableDebugMapObjPreview)
	{
		return;
	}

	ReserveGameObject(
		"Debug_MapObj", std::nullopt,
		[this](GameObject* obj)
		{
			obj->GetTransform().SetPosition(0.0f, kDebugMapObjYOffset, 0.0f);

			CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(),
				[](MeshComponent* mesh)
				{
					auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>(kDebugMapObjMeshPath.data());
					if (!meshRes)
					{
						DEBUG_LOG_FMT("[WorldScene] Debug Map.obj mesh not found: {}\n", kDebugMapObjMeshPath);
						return;
					}

					mesh->SetMeshResource(meshRes, false);

					auto		material = std::make_shared<MaterialResource>();
					const float albedo[4]{0.95f, 0.20f, 0.12f, 1.0f};
					const float emissiveColor[3]{0.95f, 0.20f, 0.12f};
					material->SetData(
						EvAsset::ShadingModel::LitPbr, MATERIAL_FLAG_DOUBLE_SIDED, albedo, 1.0f, 0.0f, emissiveColor,
						0.8f, emissiveColor, 0.8f
					);
					mesh->SetMaterialResource(0, std::move(material));
				}
			);
		}
	);
}
