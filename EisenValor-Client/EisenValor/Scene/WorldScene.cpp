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

using Vertex = EvAsset::Vertex;

namespace
{
constexpr std::string_view kTorchPreviewSphereMeshPath = "Resource/Models/Sphere.evmesh";
constexpr float			   kTorchPreviewSphereScale = 0.5f;
constexpr float			   kTorchTransportEmissionScale = 65.0f;
constexpr float			   kTorchVisibleEmissionScale = 2.5f;
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
			ReserveGameObject(
				"TorchEmitterPreview", std::nullopt,
				[this, ownerHandle = context.ownerHandle, sourceRadius = data.GetSourceRadius(),
				 transportIntensity = data.GetIntensity() * kTorchTransportEmissionScale,
				 visibleIntensity = data.GetIntensity() * kTorchVisibleEmissionScale, colorR = data.GetColor()[0],
				 colorG = data.GetColor()[1], colorB = data.GetColor()[2]](GameObject* obj)
				{
					if (auto* ownerObj = TryGetGameObject(ownerHandle))
					{
						obj->GetTransform().SetParent(ownerObj->GetTransform().GetHandle());
					}

					const float diameter = std::max(sourceRadius * 2.0f * kTorchPreviewSphereScale, 0.05f);
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
							const float emissiveColor[3]{1.0f, 0.17f, 0.18f};
							const float visibleEmissiveColor[3]{1.0f, 0.17f, 0.18f};
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


