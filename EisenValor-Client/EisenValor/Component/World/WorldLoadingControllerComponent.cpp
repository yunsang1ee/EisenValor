#include "stdafxClient.h"
#include "WorldLoadingControllerComponent.h"
#include "AudioGlobal.h"
#include "ResourceGlobal.h"
#include "SceneResource.h"
#include "Scene/WorldScene.h"
#include "StreamlineGlobal.h"

namespace
{
constexpr std::string_view kDefaultMapScenePath = "Resource/Scenes/Map.evscene";
constexpr float			   kFeatureWarmupTimeoutSeconds = 10.0f;
constexpr float			   kFeatureStabilizationTimeoutSeconds = 30.0f;
constexpr float			   kStableFrameTimeThresholdSeconds = 0.1f;
constexpr uint32_t		   kRequiredConsecutiveStableFrames = 8u;
} // namespace

void WorldLoadingControllerComponent::OnUpdate(float deltaTime)
{
	if (!m_worldScene)
	{
		return;
	}

	switch (m_phase)
	{
	case Phase::WaitForOverlayFrame:
		m_phase = Phase::LoadWorld;
		break;

	case Phase::LoadWorld:
		if (auto sceneResource =
				GLOBAL(ResourceGlobal).Load<SceneResource>(std::filesystem::path(kDefaultMapScenePath)))
		{
			m_worldScene->LoadFromSceneResource(sceneResource);
			DEBUG_LOG_FMT("[WorldScene] Loaded scene resource: {}\n", kDefaultMapScenePath);
		}
		else
		{
			m_worldScene->CreateSceneObjects();
		}
		m_phase = Phase::WaitForPendingLoads;
		break;

	case Phase::WaitForPendingLoads:
		if (!GLOBAL(ResourceGlobal).HasPendingLoads())
		{
			m_phase = Phase::WaitForRenderedWorldFrame;
		}
		break;

	case Phase::WaitForRenderedWorldFrame:
		GLOBAL(StreamlineGlobal).SetFeatureWarmupAllowed(true);
		m_warmupElapsedSeconds = 0.0f;
		DEBUG_LOG_FMT("[WorldScene] Real world render ready. Starting feature warm-up.\n");
		m_phase = Phase::WaitForRenderWarmup;
		break;

	case Phase::WaitForRenderWarmup:
	{
		auto& streamline = GLOBAL(StreamlineGlobal);
		if (!streamline.IsFeatureWarmupActive())
		{
			m_stabilizationElapsedSeconds = 0.0f;
			m_consecutiveStableFrames = 0u;
			DEBUG_LOG_FMT(
				"[WorldScene] Feature initialization reached the GPU fence. Waiting for {} consecutive frames at or "
				"below {:.0f}ms.\n",
				kRequiredConsecutiveStableFrames, kStableFrameTimeThresholdSeconds * 1000.0f
			);
			m_phase = Phase::WaitForFrameStability;
			break;
		}

		m_warmupElapsedSeconds += std::max(deltaTime, 0.0f);
		if (m_warmupElapsedSeconds >= kFeatureWarmupTimeoutSeconds)
		{
			streamline.SetEnabled(false);
			DEBUG_LOG_FMT(
				"[WorldScene] Feature initialization did not reach the GPU fence within {:.1f}s. Disabling "
				"Streamline and revealing raw gameplay fallback.\n",
				m_warmupElapsedSeconds
			);
			m_phase = Phase::RevealWorld;
		}
		break;
	}

	case Phase::WaitForFrameStability:
	{
		const float frameTimeSeconds = std::max(deltaTime, 0.0f);
		m_stabilizationElapsedSeconds += frameTimeSeconds;

		if (frameTimeSeconds > 0.0f && frameTimeSeconds <= kStableFrameTimeThresholdSeconds)
		{
			++m_consecutiveStableFrames;
		}
		else
		{
			m_consecutiveStableFrames = 0u;
		}

		if (m_consecutiveStableFrames >= kRequiredConsecutiveStableFrames)
		{
			DEBUG_LOG_FMT(
				"[WorldScene] Feature frame time stabilized for {} consecutive frames at or below {:.0f}ms after "
				"{:.1f}s. Revealing gameplay.\n",
				m_consecutiveStableFrames, kStableFrameTimeThresholdSeconds * 1000.0f, m_stabilizationElapsedSeconds
			);
			m_phase = Phase::RevealWorld;
			break;
		}

		if (m_stabilizationElapsedSeconds >= kFeatureStabilizationTimeoutSeconds)
		{
			GLOBAL(StreamlineGlobal).SetEnabled(false);
			DEBUG_LOG_FMT(
				"[WorldScene] Feature frame time did not stabilize within {:.1f}s. Disabling Streamline and "
				"revealing raw gameplay fallback.\n",
				m_stabilizationElapsedSeconds
			);
			m_phase = Phase::RevealWorld;
		}
		break;
	}

	case Phase::RevealWorld:
		GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/worldscene.wav", AudioBus::BGM, true);
		m_worldScene->DestroyGameObject(GetOwner());
		m_phase = Phase::Complete;
		break;

	case Phase::Complete:
		break;
	}
}
