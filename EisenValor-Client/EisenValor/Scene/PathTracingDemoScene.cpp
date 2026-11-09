#include "stdafxClient.h"
#include "PathTracingDemoScene.h"
#include "AudioGlobal.h"
#include "CameraComponent.h"
#include "Component/OptionsMenuComponent.h"
#include "Component/PathTracingDemoControllerComponent.h"
#include "Component/PathTracingDemoDoorComponent.h"
#include "DxRendererGlobal.h"
#include "InputGlobal.h"
#include "MaterialResource.h"
#include "MeshComponent.h"
#include "MeshResource.h"
#include "RenderPass/RaytracingPreparePass.h"
#include "ResourceGlobal.h"
#include "StreamlineGlobal.h"
#include "Transform.h"
#include "TextureResource.h"
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RenderPass/RestirDebugGlobal.h"
#endif

namespace
{
constexpr uint32_t				  kColorMasks[] = {7u, 1u, 2u, 4u, 3u, 6u, 5u};
constexpr const wchar_t*		  kColorNames[] = {L"ALL RGB",	   L"RED",			L"GREEN",	  L"BLUE",
												   L"RED + GREEN", L"GREEN + BLUE", L"BLUE + RED"};
std::shared_ptr<MaterialResource> MakeMaterial(
	DX::XMFLOAT3 color, float roughness, float metallic = 0.0f, float emission = 0.0f
)
{
	auto		material = std::make_shared<MaterialResource>();
	const float albedo[] = {color.x, color.y, color.z, 1.0f};
	const float radiance[] = {color.x, color.y, color.z};
	material->SetData(EvAsset::ShadingModel::LitPbr, 0, albedo, roughness, metallic, radiance, emission);
	return material;
}

HandleOf<GameObject> AddMesh(
	Scene&									 scene,
	const std::string&						 name,
	const std::shared_ptr<MeshResource>&	 mesh,
	const std::shared_ptr<MaterialResource>& material,
	DX::XMFLOAT3							 position,
	DX::XMFLOAT3							 scale,
	DX::XMFLOAT3							 rotation = {},
	RenderMobility							 mobility = RenderMobility::Static,
	HandleOf<GameObject>					 parent = {}
)
{
	return scene.ReserveGameObject(
		name, std::nullopt,
		[&scene, mesh, material, position, scale, rotation, mobility, parent](GameObject* obj)
		{
			auto& transform = obj->GetTransform();
			if (auto* parentObject = scene.TryGetGameObject(parent))
			{
				transform.SetParent(parentObject->GetTransform().GetHandle());
			}
			transform.SetPosition(position);
			transform.SetScale(scale);
			transform.SetRotation(rotation);
			scene.CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(),
				[mesh, material, mobility](MeshComponent* component)
				{
					component->SetMeshResource(mesh, material == nullptr);
					if (material)
					{
						for (uint32_t slot = 0; slot < component->GetMaterials().size(); ++slot)
						{
							component->SetMaterialResource(slot, material);
						}
					}
					component->SetMobility(mobility);
				}
			);
		}
	);
}

void AddFittedProp(
	Scene&									 scene,
	const std::string&						 name,
	const std::shared_ptr<MeshResource>&	 mesh,
	DX::XMFLOAT3							 center,
	DX::XMFLOAT3							 size,
	RenderMobility							 mobility = RenderMobility::Static,
	HandleOf<GameObject>					 parent = {},
	const std::shared_ptr<MaterialResource>& material = nullptr
)
{
	const auto&		   bounds = mesh->GetBounds();
	const DX::XMFLOAT3 scale = {
		size.x / (bounds.aabbmax[0] - bounds.aabbmin[0]), size.y / (bounds.aabbmax[1] - bounds.aabbmin[1]),
		size.z / (bounds.aabbmax[2] - bounds.aabbmin[2])
	};
	const DX::XMFLOAT3 position = {
		center.x - 0.5f * (bounds.aabbmin[0] + bounds.aabbmax[0]) * scale.x,
		center.y - 0.5f * (bounds.aabbmin[1] + bounds.aabbmax[1]) * scale.y,
		center.z - 0.5f * (bounds.aabbmin[2] + bounds.aabbmax[2]) * scale.z
	};
	AddMesh(scene, name, mesh, material, position, scale, {}, mobility, parent);
}
} // namespace

void PathTracingDemoScene::OnRegisterCustomComponents()
{
	RegisterComponents<OptionsMenuComponent, PathTracingDemoControllerComponent, PathTracingDemoDoorComponent>();
}

void PathTracingDemoScene::OnStartImpl()
{
	GLOBAL(AudioGlobal).StopBus(AudioBus::BGM);
	GLOBAL(InputGlobal).SetMouseLocked(false);
	m_previousPerformanceTracking = GLOBAL(DxRendererGlobal).IsPerformanceTrackingEnabled();
	GLOBAL(DxRendererGlobal).SetPerformanceTrackingEnabled(true);
	m_previousWarmupAllowed = GLOBAL(StreamlineGlobal).IsFeatureWarmupAllowed();
	GLOBAL(StreamlineGlobal).SetFeatureWarmupAllowed(false);
	GLOBAL(StreamlineGlobal).RequestFeatureWarmup();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	auto& debug = GLOBAL(RestirDebugGlobal);
	m_previousDebugOverride = debug.IsOverrideActive();
	m_previousDebugSource = static_cast<uint32_t>(debug.GetSource());
	m_previousDebugView = static_cast<uint32_t>(debug.GetView());
	m_previousSecondaryNee = debug.IsSecondaryNeeEnabled();
	if (!m_previousSecondaryNee)
	{
		debug.ToggleSecondaryNee();
	}
	m_previousCameraFrozen = GLOBAL(DxRendererGlobal).IsDebugCameraFrozen();
	if (m_previousCameraFrozen)
	{
		GLOBAL(DxRendererGlobal).ToggleDebugCameraFreeze();
	}
#endif
	auto* prepare = static_cast<RaytracingPreparePass*>(GLOBAL(DxRendererGlobal).GetRenderPass("RaytracingPrepare"));
	m_hasPreviousSettings = prepare != nullptr;
	if (prepare)
	{
		m_previousSettings = prepare->GetControls().GetSettings();
		RaytracingSettings settings;
		settings.usePathTracing = true;
		settings.useRestirPT = true;
		settings.useDayEnvironment = false;
		settings.usePhysicalRenderingBaseline = true;
		settings.restirPrimarySpp = 2;
		prepare->GetControls().SetSettings(settings);
	}

	CreateExhibits();
	CreateColorRoom();
	ReserveGameObject(
		"DemoCamera", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<CameraComponent>(
				obj->GetHandle(), [](CameraComponent* camera) { camera->SetAsMainCamera(); }
			);
			CreateComponentWithInit<PathTracingDemoControllerComponent>(
				obj->GetHandle(),
				[this](PathTracingDemoControllerComponent* controller) { controller->SetDoorObject(m_doorRoot); }
			);
		}
	);
	ReserveGameObject(
		"DemoOptions", std::nullopt,
		[this](GameObject* obj) { CreateComponent<OptionsMenuComponent>(obj->GetHandle()); }
	);
	DEBUG_LOG_FMT("[PathTracingDemo] Dragon room loaded. WASD fly, RMB look, Space pause door, O open/close.\n");
}

void PathTracingDemoScene::CreateExhibits()
{
	auto&	   resources = GLOBAL(ResourceGlobal);
	const auto box = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/Box.evmesh");
	const auto sphere = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/Sphere.evmesh");
	const auto dragon = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/StanfordDragon.evmesh");
	const auto panel = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/LightPanel.evmesh");
	const auto castleDoor =
		resources.Load<MeshResource>(L"Resource/Models/Map/Map/MCastle_ArchDoors_02_LOD0_b6abc91c.evmesh");
	const auto pillar =
		resources.Load<MeshResource>(L"Resource/Models/Map/Map/MCastle_GiantPillar_01_LOD2_e7b19f7d.evmesh");
	if (!box || !sphere || !dragon || !panel || !castleDoor || !pillar)
	{
		throw std::runtime_error("Path tracing demo assets are missing. See Tools/PathTracingDemo.");
	}

	const auto plaster = MakeMaterial({0.72f, 0.72f, 0.68f}, 0.82f);
	const auto porcelain = MakeMaterial({0.8f, 0.8f, 0.78f}, 0.3f);
	const auto floor = MakeMaterial({0.42f, 0.44f, 0.46f}, 0.22f);
	const auto red = MakeMaterial({0.62f, 0.055f, 0.035f}, 0.85f);
	const auto blue = MakeMaterial({0.035f, 0.15f, 0.62f}, 0.85f);
	const auto charcoal = MakeMaterial({0.045f, 0.05f, 0.06f}, 0.5f);
	const auto bronze = MakeMaterial({0.88f, 0.49f, 0.18f}, 0.23f, 1.0f);
	const auto chrome = MakeMaterial({0.92f, 0.93f, 0.95f}, 0.075f, 1.0f);
	const auto mirror = MakeMaterial({0.94f, 0.94f, 0.94f}, 0.035f, 1.0f);
	const auto woodSeal = MakeMaterial({0.17f, 0.07f, 0.025f}, 0.65f);
	const auto whiteLight = MakeMaterial({1.0f, 0.94f, 0.83f}, 1, 0, 24);
	const auto warmLight = MakeMaterial({1.0f, 0.18f, 0.045f}, 1, 0, 40);
	const auto coolLight = MakeMaterial({0.07f, 0.3f, 1.0f}, 1, 0, 40);

	AddMesh(*this, "RoomFloor", box, floor, {0, -0.15f, 1.5f}, {12.4f, 0.3f, 19.4f});
	AddMesh(*this, "RoomCeiling", box, plaster, {0, 6.15f, 1.5f}, {12.4f, 0.3f, 19.4f});
	AddMesh(*this, "RoomFloor", box, floor, {0, -0.15f, 1.5f}, {12.4f, 0.3f, 19.4f});
	AddMesh(*this, "RoomCeiling", box, plaster, {0, 6.15f, 1.5f}, {12.4f, 0.3f, 19.4f});
	AddMesh(*this, "RedBounceWall", box, red, {-6.15f, 3, 1.5f}, {0.3f, 6, 19});
	AddMesh(*this, "BluePassageLintel", box, blue, {6.15f, 5.3f, -5}, {0.3f, 1.4f, 2});
	AddMesh(*this, "BlueBounceWallFront", box, blue, {6.15f, 3, -7}, {0.3f, 6, 2});
	AddMesh(*this, "BlueBounceWallBack", box, blue, {6.15f, 3, 3.5f}, {0.3f, 6, 15});
	AddMesh(*this, "BluePassageLintel", box, blue, {6.15f, 5.3f, -5}, {0.3f, 1.4f, 2});
	AddMesh(*this, "LightChamberBack", box, plaster, {0, 3, 11.15f}, {12.4f, 6, 0.3f});
	AddMesh(*this, "CameraWall", box, plaster, {0, 3, -8.15f}, {12.4f, 6, 0.3f});
	AddMesh(*this, "DoorWallLeft", box, plaster, {-3.9f, 3, 5.1f}, {4.2f, 6, 0.4f});
	AddMesh(*this, "DoorWallRight", box, plaster, {3.9f, 3, 5.1f}, {4.2f, 6, 0.4f});
	AddMesh(*this, "DoorWallLintel", box, plaster, {0, 5.3f, 5.1f}, {3.6f, 1.4f, 0.4f});

	AddMesh(*this, "OutsideWhiteEmitter", panel, whiteLight, {0, 5.8f, 8.0f}, {6, 1, 4});
	AddMesh(*this, "OutsideWarmEmitter", panel, warmLight, {-2.05f, 3, 10.85f}, {3.8f, 1, 5}, {90, 0, 0});
	AddMesh(*this, "OutsideCoolEmitter", panel, coolLight, {2.05f, 3, 10.85f}, {3.8f, 1, 5}, {90, 0, 0});

	AddFittedProp(*this, "CastlePillarLeft", pillar, {-3.0f, 2.45f, 4.6f}, {1.0f, 4.9f, 1.0f});
	AddFittedProp(*this, "CastlePillarRight", pillar, {3.0f, 2.45f, 4.6f}, {1.0f, 4.9f, 1.0f});
	AddMesh(*this, "DoorFrameLeft", box, charcoal, {-1.91f, 2.35f, 4.84f}, {0.22f, 4.7f, 0.22f});
	AddMesh(*this, "DoorFrameRight", box, charcoal, {1.91f, 2.35f, 4.84f}, {0.22f, 4.7f, 0.22f});
	AddMesh(*this, "DoorFrameTop", box, charcoal, {0, 4.71f, 4.84f}, {4.04f, 0.22f, 0.22f});
	m_doorRoot = ReserveGameObject(
		"AnimatedDoorHinge", std::nullopt,
		[this](GameObject* obj)
		{
			obj->GetTransform().SetPosition(-1.8f, 0, 5.1f);
			CreateComponent<PathTracingDemoDoorComponent>(obj->GetHandle());
		}
	);
	AddMesh(
		*this, "DoorLightSeal", box, woodSeal, {1.8f, 2.3f, 0.1f}, {3.68f, 4.64f, 0.16f}, {}, RenderMobility::Movable,
		m_doorRoot
	);
	AddFittedProp(
		*this, "CastleDoorLeaf", castleDoor, {1.8f, 2.3f, -0.06f}, {3.6f, 4.6f, 0.25f}, RenderMobility::Movable,
		m_doorRoot
	);
	AddMesh(
		*this, "DoorHandle", sphere, bronze, {3.15f, 2.2f, -0.28f}, {0.22f, 0.22f, 0.22f}, {}, RenderMobility::Movable,
		m_doorRoot
	);

	AddMesh(*this, "BronzeDragonPlinth", box, charcoal, {0, 0.35f, 0.8f}, {4.5f, 0.7f, 2.7f});
	AddMesh(*this, "BronzeStanfordDragon", dragon, bronze, {0, 0.7f, 0.8f}, {2.65f, 2.65f, 2.65f}, {0, 15, 0});
	AddMesh(*this, "PorcelainDragonPlinth", box, plaster, {-3.8f, 0.25f, 1.6f}, {2.8f, 0.5f, 1.8f});
	AddMesh(
		*this, "PorcelainStanfordDragon", dragon, porcelain, {-3.8f, 0.5f, 1.6f}, {1.75f, 1.75f, 1.75f}, {0, 15, 0}
	);
	AddMesh(*this, "ChromeDragonPlinth", box, charcoal, {3.8f, 0.25f, 1.6f}, {2.8f, 0.5f, 1.8f});
	AddMesh(*this, "ChromeStanfordDragon", dragon, chrome, {3.8f, 0.5f, 1.6f}, {1.75f, 1.75f, 1.75f}, {0, -15, 0});

	AddMesh(*this, "MirrorFrame", box, charcoal, {5.9f, 2.6f, -0.6f}, {0.18f, 4.6f, 5.6f});
	AddMesh(*this, "MirrorFrame", box, charcoal, {5.9f, 2.6f, -0.6f}, {0.18f, 4.6f, 5.6f});
	AddMesh(*this, "SideMirror", box, mirror, {5.79f, 2.6f, -0.6f}, {0.025f, 4.2f, 5.2f});
	AddMesh(*this, "BounceShelf", box, plaster, {-5.0f, 2.75f, -1.0f}, {2.0f, 0.2f, 2.2f});
	AddMesh(*this, "ShelteredReceiver", sphere, porcelain, {-5.0f, 0.65f, -1.0f}, {1.3f, 1.3f, 1.3f});

	AddMesh(*this, "RoughnessBench", box, charcoal, {0, 0.22f, -2.8f}, {6.3f, 0.44f, 1.3f});
	constexpr float roughness[] = {0.07f, 0.2f, 0.4f, 0.7f, 1.0f};
	for (size_t index = 0; index < std::size(roughness); ++index)
	{
		const auto metal = MakeMaterial({0.82f, 0.85f, 0.9f}, roughness[index], 1.0f);
		AddMesh(
			*this, std::format("RoughnessSphere{}", index), sphere, metal,
			{-2.4f + 1.2f * static_cast<float>(index), 0.94f, -2.8f}, {1, 1, 1}
		);
	}
}

void PathTracingDemoScene::CreateColorRoom()
{
	auto&	   resources = GLOBAL(ResourceGlobal);
	const auto box = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/Box.evmesh");
	const auto sphere = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/Sphere.evmesh");
	const auto dragon = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/StanfordDragon.evmesh");
	const auto panel = resources.Load<MeshResource>(L"Resource/Models/PathTracingDemo/LightPanel.evmesh");
	const auto floorTile =
		resources.Load<MeshResource>(L"Resource/Models/Map/Map/MCastle_TileSlabs_01_LOD2_1d5ed339.evmesh");
	const auto pillar = resources.Load<MeshResource>(
		L"Resource/Models/Map/Map/MCastle_MasonrySupports_FlatPillar_01_LOD1_1b3f9a2d.evmesh"
	);
	const auto lowWall =
		resources.Load<MeshResource>(L"Resource/Models/Map/Map/MCastle_LowWalls_01_LOD0_e12ef6a3.evmesh");
	const auto barrel =
		resources.Load<MeshResource>(L"Resource/Models/Map/Map/Medieval_Containers_Barrel_LOD2_62242ddf.evmesh");
	const auto torch =
		resources.Load<MeshResource>(L"Resource/Models/Map/Map/MCastle_Fixtures_Torch_Standing_LOD2_00133a7f.evmesh");
	const auto washedAlbedo =
		resources.Load<TextureResource>(L"Resource/Texture/PathTracingDemo/WashedStoneAlbedo.evtex");
	const auto polishedOrm =
		resources.Load<TextureResource>(L"Resource/Texture/PathTracingDemo/PolishedStoneORMS.evtex");
	const auto groutEmission =
		resources.Load<TextureResource>(L"Resource/Texture/PathTracingDemo/StoneGroutEmission.evtex");
	const auto stoneNormal =
		resources.Load<TextureResource>(L"Resource/Texture/Map/Map/MCastle_StoneSlabs_Indoor_Normal_32c3ebe0.evtex");
	if (!box || !sphere || !dragon || !panel || !floorTile || !pillar || !lowWall || !barrel || !torch ||
		!washedAlbedo || !polishedOrm || !groutEmission || !stoneNormal)
	{
		throw std::runtime_error("Color room assets missing. Run Tools/PathTracingDemo/generate_texture_variants.py.");
	}

	m_colorLights.clear();
	m_colorPreset = 0;
	m_unlitFixture = MakeMaterial({0.025f, 0.025f, 0.025f}, 0.7f);
	const auto		   white = MakeMaterial({0.78f, 0.78f, 0.78f}, 0.7f);
	const auto		   chalk = MakeMaterial({0.83f, 0.83f, 0.83f}, 0.9f);
	const auto		   dark = MakeMaterial({0.06f, 0.065f, 0.07f}, 0.6f);
	const auto		   chrome = MakeMaterial({0.93f, 0.93f, 0.93f}, 0.09f, 1);
	const float		   albedo[] = {1, 1, 1, 1};
	const float		   noEmission[] = {0, 0, 0};
	constexpr uint32_t stoneFlags =
		MATERIAL_FLAG_USE_ALBEDO_MAP | MATERIAL_FLAG_USE_NORMAL_MAP | MATERIAL_FLAG_USE_ORM_MAP;
	auto stone = std::make_shared<MaterialResource>();
	stone->SetData(EvAsset::ShadingModel::LitPbr, stoneFlags, albedo, 0.35f, 0, noEmission, 0);
	stone->SetTexture("ALBD", washedAlbedo);
	stone->SetTexture("NRML", stoneNormal);
	stone->SetTexture("ORMS", polishedOrm);

	AddMesh(*this, "PassageFloor", box, dark, {8, -0.15f, -3}, {4.3f, 0.3f, 6.3f});
	AddMesh(*this, "PassageFloor", box, dark, {8, -0.15f, -3}, {4.3f, 0.3f, 6.3f});
	AddMesh(*this, "PassageCeiling", box, dark, {8, 4.75f, -3}, {4.3f, 0.3f, 6.3f});
	AddMesh(*this, "PassageFront", box, dark, {8, 2.3f, -6.15f}, {4.3f, 4.6f, 0.3f});
	AddMesh(*this, "PassageBack", box, dark, {8, 2.3f, 0.15f}, {4.3f, 4.6f, 0.3f});
	AddMesh(*this, "PassageLightBaffle", box, dark, {8, 2.3f, -4.2f}, {0.2f, 4.6f, 3.6f});

	AddMesh(*this, "ColorRoomFloorSeal", box, dark, {17, -0.2f, -1.25f}, {14.4f, 0.3f, 13.9f});
	AddMesh(*this, "ColorRoomCeiling", box, white, {17, 6.15f, -1.25f}, {14.4f, 0.3f, 13.9f});
	AddMesh(*this, "ColorRoomFront", box, white, {17, 3, -8.15f}, {14.4f, 6, 0.3f});
	AddMesh(*this, "ColorRoomBack", box, white, {17, 3, 5.65f}, {14.4f, 6, 0.3f});
	AddMesh(*this, "ColorRoomRight", box, white, {24.15f, 3, -1.25f}, {0.3f, 6, 13.5f});
	AddMesh(*this, "ColorRoomLeftFront", box, white, {10.15f, 3, -5}, {0.3f, 6, 6});
	AddMesh(*this, "ColorRoomLeftBack", box, white, {10.15f, 3, 2.75f}, {0.3f, 6, 5.5f});
	AddMesh(*this, "ColorRoomPortalLintel", box, white, {10.15f, 5.3f, -1}, {0.3f, 1.4f, 2});

	for (int z = 0; z < 4; ++z)
	{
		for (int x = 0; x < 4; ++x)
		{
			AddFittedProp(
				*this, std::format("ColorStoneFloor{}_{}", x, z), floorTile,
				{11.85f + 3.4f * x, 0.04f, -6.3f + 3.4f * z}, {3.4f, 0.18f, 3.4f}, RenderMobility::Static, {}, stone
			);
		}
	}
	for (int side = 0; side < 2; ++side)
	{
		for (int row = 0; row < 2; ++row)
		{
			AddFittedProp(
				*this, std::format("ColorCastlePillar{}_{}", side, row), pillar,
				{11.05f + 11.9f * side, 2.45f, -3.2f + 6.8f * row}, {1.1f, 4.9f, 0.65f}
			);
		}
	}
	AddFittedProp(*this, "ColorCastleBackDado", lowWall, {17, 1.1f, 5.27f}, {13.3f, 2.2f, 0.35f});
	for (int index = 0; index < 5; ++index)
	{
		const bool left = index < 3;
		AddFittedProp(
			*this, std::format("ColorWoodBarrel{}", index), barrel,
			{left ? 11.5f + 0.95f * index : 21.7f + 0.95f * (index - 3), 0.7f, left ? -6.45f : 3.8f}, {1.1f, 1.4f, 1.1f}
		);
	}

	struct ColorSource
	{
		DX::XMFLOAT3 color;
		DX::XMFLOAT3 position;
		DX::XMFLOAT3 size;
		DX::XMFLOAT3 rotation;
		DX::XMFLOAT3 torchPosition;
		float		 intensity;
		uint32_t	 mask;
	};
	
	const ColorSource sources[] = {
		{{1.0f, 0.025f, 0.015f}, {10.45f, 3.6f, 1.0f}, {2.6f, 1, 4}, {0, 0, 90}, {12.7f, 1.1f, 0.4f}, 22, 1},
		{{0.025f, 1.0f, 0.055f}, {17, 4.0f, 5.42f}, {4, 1, 2.6f}, {90, 0, 0}, {17, 1.1f, 4.2f}, 14, 2},
		{{0.04f, 0.12f, 1.0f}, {23.7f, 3.6f, 1.0f}, {2.6f, 1, 4}, {0, 0, -90}, {21.3f, 1.1f, 0.4f}, 28, 4}
	};
	for (size_t index = 0; index < std::size(sources); ++index)
	{
		const auto& source = sources[index];
		const auto	emitter = MakeMaterial(source.color, 1, 0, source.intensity);
		const auto	light = AddMesh(
			 *this, std::format("ColorAreaLight{}", index), panel, emitter, source.position, source.size, source.rotation
		 );
		m_colorLights.push_back({light, emitter, source.mask});
		AddFittedProp(*this, std::format("ColorCastleTorch{}", index), torch, source.torchPosition, {0.8f, 2.2f, 0.8f});
		const auto glow = MakeMaterial(source.color, 1, 0, 0.5f);
		const auto lamp = AddMesh(
			*this, std::format("ColorTorchGlow{}", index), box, glow,
			{source.torchPosition.x, 2.28f, source.torchPosition.z}, {0.25f, 0.24f, 0.25f}
		);
		m_colorLights.push_back({lamp, glow, source.mask});

		auto		carved = std::make_shared<MaterialResource>();
		const float emission[] = {source.color.x, source.color.y, source.color.z};
		carved->SetData(
			EvAsset::ShadingModel::LitPbr, stoneFlags | MATERIAL_FLAG_EMISSIVE_MAP, albedo, 0.35f, 0, emission, 1.2f
		);
		carved->SetTexture("ALBD", washedAlbedo);
		carved->SetTexture("NRML", stoneNormal);
		carved->SetTexture("ORMS", polishedOrm);
		carved->SetTexture("EMSV", groutEmission);
		const auto frieze = AddMesh(
			*this, std::format("ColorGlowingStone{}", index), panel, carved,
			{12.8f + 4.2f * static_cast<float>(index), 1.25f, 5.04f}, {3.6f, 1, 1.5f}, {90, 0, 0}
		);
		m_colorLights.push_back({frieze, carved, source.mask});
	}

	AddMesh(*this, "ColorMixReceiver", box, chalk, {17, 0.25f, -0.1f}, {6.4f, 0.22f, 5.0f});
	AddMesh(*this, "ColorWhiteDragon", dragon, white, {17, 0.36f, 0.8f}, {2.0f, 2.0f, 2.0f}, {0, 15, 0});
	AddMesh(*this, "ColorShadowColumnA", box, chalk, {15.0f, 1.16f, -0.9f}, {0.24f, 1.6f, 0.24f});
	AddMesh(*this, "ColorShadowColumnB", box, chalk, {18.9f, 1.46f, -0.7f}, {0.32f, 2.2f, 0.32f});
	AddMesh(*this, "ColorMirrorPlinth", box, dark, {21.1f, 0.35f, -2.1f}, {2, 0.7f, 2});
	AddMesh(*this, "ColorMirrorSphere", sphere, chrome, {21.1f, 1.5f, -2.1f}, {1.6f, 1.6f, 1.6f});
	AddMesh(*this, "ColorIndirectCanopy", box, white, {12.1f, 2.7f, 2.4f}, {2.8f, 0.18f, 2.3f});
	AddMesh(*this, "ColorShelteredSphere", sphere, chalk, {12.1f, 0.8f, 2.4f}, {1.3f, 1.3f, 1.3f});
}

const wchar_t* PathTracingDemoScene::GetColorLightingLabel() const
{
	return kColorNames[m_colorPreset];
}

void PathTracingDemoScene::CycleColorLighting()
{
	if (GLOBAL(ResourceGlobal).HasPendingLoads())
	{
		return;
	}
	m_colorPreset = (m_colorPreset + 1) % static_cast<uint32_t>(std::size(kColorMasks));
	for (const auto& light : m_colorLights)
	{
		if (auto* object = TryGetGameObject(light.object))
		{
			if (auto* mesh = object->GetComponent<MeshComponent>())
			{
				mesh->SetMaterialResource(
					0, (kColorMasks[m_colorPreset] & light.mask) ? light.material : m_unlitFixture
				);
			}
		}
	}
	GLOBAL(StreamlineGlobal).RequestHistoryReset();
}

void PathTracingDemoScene::OnEndImpl()
{
	GLOBAL(InputGlobal).SetMouseLocked(false);
	GLOBAL(DxRendererGlobal).SetPerformanceTrackingEnabled(m_previousPerformanceTracking);
	GLOBAL(StreamlineGlobal).SetFeatureWarmupAllowed(m_previousWarmupAllowed);
	if (m_hasPreviousSettings)
	{
		if (auto* prepare =
				static_cast<RaytracingPreparePass*>(GLOBAL(DxRendererGlobal).GetRenderPass("RaytracingPrepare")))
		{
			prepare->GetControls().SetSettings(m_previousSettings);
		}
		m_hasPreviousSettings = false;
	}
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	auto& debug = GLOBAL(RestirDebugGlobal);
	debug.DisableOverride();
	if (m_previousDebugOverride)
	{
		debug.StepSource(static_cast<int32_t>(m_previousDebugSource));
		if (m_previousDebugView != 0)
		{
			debug.StepView(static_cast<int32_t>(m_previousDebugView));
		}
	}
	if (debug.IsSecondaryNeeEnabled() != m_previousSecondaryNee)
	{
		debug.ToggleSecondaryNee();
	}
	if (GLOBAL(DxRendererGlobal).IsDebugCameraFrozen() != m_previousCameraFrozen)
	{
		GLOBAL(DxRendererGlobal).ToggleDebugCameraFreeze();
	}
#endif
	DEBUG_LOG_FMT("[PathTracingDemo] Renderer settings restored.\n");
}
