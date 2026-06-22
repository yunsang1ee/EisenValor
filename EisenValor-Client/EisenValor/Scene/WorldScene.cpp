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
#include "Component/World/WorldLoadingControllerComponent.h"
#include "Component/World/QuestProgressComponent.h"
#include "Component/World/QuestUIComponent.h"

// Engine
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "RectTransformComponent.h"
#include "TextUIComponent.h"
#include "AudioGlobal.h"

#include "Transform.h"
#include "MeshResource.h"

// Resource
#include "NetworkGlobal.h"
#include "ResourceGlobal.h"
#include "SkinnedMeshResource.h"
#include "TextureResource.h"

#include "MeshComponent.h"
#include "MaterialResource.h"
#include "RaytracingCommon.h"

#include <array>

using Vertex = EvAsset::Vertex;

namespace
{
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
		AttackRangeDebugComponent, WorldSceneControllerComponent, FootIKComponent,
		WorldLoadingControllerComponent, QuestUIComponent, QuestProgressComponent>();
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
	GLOBAL(AudioGlobal).SetBusVolume(AudioBus::BGM, 0.1f);

	if (!GLOBAL(NetBridge::NetworkGlobal).Init("127.0.0.1", G_GAME_SERVER_PORT))
	{
		DEBUG_LOG_FMT("[WorldScene] Failed to connect game server.\n");
	}

	ReserveGameObject(
		"WorldLoadingOverlay", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 0.0f}, {1.0f, 1.0f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({0.0f, 0.0f});
					rect->SetOffsetMax({0.0f, 0.0f});
				}
			);

			CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(
						L"Resource\\Texture\\Scene\\loadingscene.evtex");
					image->SetNormalTextureResource(texture);
					image->SetOrder(100000);
				}
			);

			CreateComponent<WorldLoadingControllerComponent>(obj->GetHandle());
		}
	);

#if 0
	if (false)
	{
		CreateSceneObjects();

		// ?쒕쾭 ?놁씠 ?뚯뒪?몃? ?꾪븳 ?ㅽ듃?덉뒪 ?뚯뒪???ㅻ툕?앺듃 ?앹꽦
		// ReserveGameObject(
		//	"StressTester", std::nullopt,
		//	[this](GameObject* obj) { CreateComponent<StressTestComponent>(obj->GetHandle()); }
		//);
	}

#endif

	ReserveGameObject(
		"WorldSceneController", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<WorldSceneControllerComponent>(
				obj->GetHandle(), [](WorldSceneControllerComponent* login) {}
			);
		}
	);

	ReserveGameObject(
		"QuestMessage", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.25f}, {0.5f, 0.25f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-320.0f, -80.0f});
					rect->SetOffsetMax({320.0f, 240.0f});
				}
			);

			CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(
						L"Resource\\Texture\\quest.evtex");
					image->SetNormalTextureResource(texture);
					image->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
					image->SetOrder(99998);
				}
			);

			CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"WASD\uB85C \uC774\uB3D9\uD558\uC138\uC694");
					text->SetFontSize(32.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
					text->SetOrder(99999);
				}
			);

			CreateComponent<QuestUIComponent>(obj->GetHandle());
			CreateComponent<QuestProgressComponent>(obj->GetHandle());
		}
	);

	ReserveGameObject(
		"OccupationGaugeRoot", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 0.0f}, {0.0f, 0.0f});
					rect->SetPivot({0.0f, 0.0f});
					rect->SetOffsetMin({24.0f, 24.0f});
					rect->SetOffsetMax({344.0f, 48.0f});
				}
			);

			CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.05f, 0.05f, 0.05f, 0.9f});
					image->SetOrder(99990);
				}
			);

			const auto rootHandle = obj->GetHandle();

			ReserveGameObject(
				"OccupationGaugeBlue", std::nullopt,
				[this, rootHandle](GameObject* fillObj)
				{
					fillObj->GetTransform().SetParent(
						TryGetGameObject(rootHandle)->GetComponentHandle<Transform>()
					);

					CreateComponentWithInit<RectTransformComponent>(
						fillObj->GetHandle(),
						[](RectTransformComponent* rect)
						{
							rect->SetAnchors({0.0f, 0.0f}, {0.5f, 1.0f});
							rect->SetPivot({0.0f, 0.5f});
							rect->SetOffsetMin({2.0f, 2.0f});
							rect->SetOffsetMax({-1.0f, -2.0f});
						}
					);

					CreateComponentWithInit<ImageUIComponent>(
						fillObj->GetHandle(),
						[](ImageUIComponent* image)
						{
							image->SetNormalColor({0.15f, 0.35f, 1.0f, 1.0f});
							image->SetOrder(99991);
						}
					);
				}
			);

			ReserveGameObject(
				"OccupationGaugeRed", std::nullopt,
				[this, rootHandle](GameObject* fillObj)
				{
					fillObj->GetTransform().SetParent(
						TryGetGameObject(rootHandle)->GetComponentHandle<Transform>()
					);

					CreateComponentWithInit<RectTransformComponent>(
						fillObj->GetHandle(),
						[](RectTransformComponent* rect)
						{
							rect->SetAnchors({0.5f, 0.0f}, {1.0f, 1.0f});
							rect->SetPivot({1.0f, 0.5f});
							rect->SetOffsetMin({1.0f, 2.0f});
							rect->SetOffsetMax({-2.0f, -2.0f});
						}
					);

					CreateComponentWithInit<ImageUIComponent>(
						fillObj->GetHandle(),
						[](ImageUIComponent* image)
						{
							image->SetNormalColor({1.0f, 0.2f, 0.2f, 1.0f});
							image->SetOrder(99991);
						}
					);
				}
			);

			ReserveGameObject(
				"OccupationGaugeCenterLine", std::nullopt,
				[this, rootHandle](GameObject* lineObj)
				{
					lineObj->GetTransform().SetParent(
						TryGetGameObject(rootHandle)->GetComponentHandle<Transform>()
					);

					CreateComponentWithInit<RectTransformComponent>(
						lineObj->GetHandle(),
						[](RectTransformComponent* rect)
						{
							rect->SetAnchors({0.5f, 0.0f}, {0.5f, 1.0f});
							rect->SetPivot({0.5f, 0.5f});
							rect->SetOffsetMin({-1.0f, 2.0f});
							rect->SetOffsetMax({1.0f, -2.0f});
						}
					);

					CreateComponentWithInit<ImageUIComponent>(
						lineObj->GetHandle(),
						[](ImageUIComponent* image)
						{
							image->SetNormalColor({1.0f, 1.0f, 1.0f, 0.85f});
							image->SetOrder(99992);
						}
					);
				}
			);
		}
	);

	ReserveGameObject(
		"RemainingTimeText", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.0f});
					rect->SetOffsetMin({-120.0f, 16.0f});
					rect->SetOffsetMax({120.0f, 60.0f});
				}
			);

			CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"30:00");
					text->SetFontSize(28.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
					text->SetOrder(99995);
				}
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
	GLOBAL(AudioGlobal).StopBus(AudioBus::BGM);
	GLOBAL(AudioGlobal).SetBusVolume(AudioBus::BGM, 1.0f);
	DEBUG_LOG_FMT("[WorldScene] OnEnd called\n");
}


