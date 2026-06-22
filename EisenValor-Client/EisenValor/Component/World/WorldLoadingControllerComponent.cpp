#include "stdafxClient.h"
#include "WorldLoadingControllerComponent.h"
#include "AudioGlobal.h"
#include "ResourceGlobal.h"
#include "SceneGlobal.h"
#include "SceneResource.h"
#include "Scene/WorldScene.h"

namespace
{
constexpr std::string_view kDefaultMapScenePath = "Resource/Scenes/Map.evscene";
}

void WorldLoadingControllerComponent::OnUpdate(float deltaTime)
{
	(void)deltaTime;

	auto* worldScene = dynamic_cast<WorldScene*>(GLOBAL(SceneGlobal).GetActiveScene());
	if (!worldScene)
	{
		return;
	}

	switch (m_phase)
	{
	case Phase::WaitForOverlayFrame:
		m_phase = Phase::LoadWorld;
		break;

	case Phase::LoadWorld:
		if (auto sceneResource = GLOBAL(ResourceGlobal).Load<SceneResource>(
				std::filesystem::path(kDefaultMapScenePath)))
		{
			worldScene->LoadFromSceneResource(sceneResource);
			DEBUG_LOG_FMT("[WorldScene] Loaded scene resource: {}\n", kDefaultMapScenePath);
		}
		else
		{
			worldScene->CreateSceneObjects();
		}
		m_phase = Phase::WaitForPendingLoads;
		break;

	case Phase::WaitForPendingLoads:
		if (!GLOBAL(ResourceGlobal).HasPendingLoads())
		{
			m_phase = Phase::WaitForFinalFrame;
		}
		break;

	case Phase::WaitForFinalFrame:
		GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/worldscene.wav", AudioBus::BGM, true);
		worldScene->DestroyGameObject(GetOwner());
		m_phase = Phase::Complete;
		break;

	case Phase::Complete:
		break;
	}
}
