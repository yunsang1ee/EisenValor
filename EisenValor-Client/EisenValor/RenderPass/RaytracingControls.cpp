#include "stdafxClient.h"
#include "RaytracingControls.h"
#include "InputGlobal.h"
#include "DxRendererGlobal.h"
#include "RenderData/RaytracingOutputRenderData.h"
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RestirDebugGlobal.h"
#endif
#if defined(ENABLE_STREAMLINE)
#include "StreamlineGlobal.h"
#endif

void RaytracingControls::TogglePathTracing()
{
	m_settings.usePathTracing = !m_settings.usePathTracing;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	auto& debug = GLOBAL(RestirDebugGlobal);
	if (debug.IsOverrideActive())
	{
		debug.DisableOverride();
		++m_settings.restirHistoryGeneration;
#if defined(ENABLE_STREAMLINE)
		GLOBAL(StreamlineGlobal).RequestHistoryReset();
#endif
	}
#endif
}

void RaytracingControls::ToggleRestirPT()
{
	m_settings.useRestirPT = !m_settings.useRestirPT;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	auto& debug = GLOBAL(RestirDebugGlobal);
	debug.DisableOverride();
#endif
	++m_settings.restirHistoryGeneration;
#if defined(ENABLE_STREAMLINE)
	GLOBAL(StreamlineGlobal).RequestHistoryReset();
#endif
}

void RaytracingControls::ToggleDayEnvironment()
{
	m_settings.useDayEnvironment = !m_settings.useDayEnvironment;
	++m_settings.restirHistoryGeneration;
}

void RaytracingControls::TogglePhysicalRenderingBaseline()
{
	m_settings.usePhysicalRenderingBaseline = !m_settings.usePhysicalRenderingBaseline;
	++m_settings.restirHistoryGeneration;
#if defined(ENABLE_STREAMLINE)
	GLOBAL(StreamlineGlobal).RequestHistoryReset();
#endif
	DEBUG_LOG_FMT(
		"[DXR.Debug] Rendering look: {}\n",
		m_settings.usePhysicalRenderingBaseline ? "PHYSICAL_BASELINE" : "ART_DIRECTED"
	);
}

void RaytracingControls::Update()
{
#if defined(ENABLE_RENDER_DEBUG_VIEWS) || defined(PROFILE_BUILD)
	auto& input = GLOBAL(InputGlobal);
#endif
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	auto&		   debug = GLOBAL(RestirDebugGlobal);
	const bool	   previousOverrideActive = debug.IsOverrideActive();
	const auto	   previousSource = debug.GetSource();
	const uint64_t previousDebugRevision = debug.GetRevision();
#endif
#if defined(ENABLE_RENDER_DEBUG_VIEWS) || defined(PROFILE_BUILD)
	if (input.GetInputDown(VK_F8))
	{
		const int32_t profileStep = input.GetInput(VK_SHIFT) ? -1 : 1;
		if (input.GetInput(VK_MENU))
		{
			m_settings.restirPrimarySpp =
				profileStep > 0 ? (m_settings.restirPrimarySpp == 4u ? 1u : m_settings.restirPrimarySpp * 2u)
								: (m_settings.restirPrimarySpp == 1u ? 4u : m_settings.restirPrimarySpp / 2u);
			DEBUG_LOG_FMT("[ReSTIR] SPP={} (Alt+F8: 1/2/4)\n", m_settings.restirPrimarySpp);
		}
		else if (input.GetInput(VK_CONTROL))
		{
			constexpr uint32_t profileStages[] = {
				RESTIR_EMISSIVE_PROFILE_SURFACE_ONLY, RESTIR_EMISSIVE_PROFILE_SAMPLE_NO_VISIBILITY,
				RESTIR_EMISSIVE_PROFILE_FULL
			};
			int32_t currentStageIndex = 0;
			for (int32_t index = 0; index < static_cast<int32_t>(std::size(profileStages)); ++index)
			{
				if (profileStages[index] == m_settings.restirEmissiveProfileStage)
				{
					currentStageIndex = index;
					break;
				}
			}
			const int32_t stageCount = static_cast<int32_t>(std::size(profileStages));
			currentStageIndex = (currentStageIndex + profileStep + stageCount) % stageCount;
			m_settings.restirEmissiveProfileStage = profileStages[currentStageIndex];
			m_settings.restirCandidateMask = RESTIR_CANDIDATE_ALL;
		}
		else
		{
			constexpr uint32_t candidateModes[] = {
				RESTIR_CANDIDATE_PATH, RESTIR_CANDIDATE_PATH | RESTIR_CANDIDATE_SUN_NEE, RESTIR_CANDIDATE_ALL
			};
			int32_t currentModeIndex = 0;
			for (int32_t index = 0; index < static_cast<int32_t>(std::size(candidateModes)); ++index)
			{
				if (candidateModes[index] == m_settings.restirCandidateMask)
				{
					currentModeIndex = index;
					break;
				}
			}
			const int32_t modeCount = static_cast<int32_t>(std::size(candidateModes));
			currentModeIndex = (currentModeIndex + profileStep + modeCount) % modeCount;
			m_settings.restirCandidateMask = candidateModes[currentModeIndex];
		}
		++m_settings.profileRevision;

		++m_settings.restirHistoryGeneration;
#if defined(ENABLE_STREAMLINE)
		GLOBAL(StreamlineGlobal).RequestHistoryReset();
#endif
	}
#endif
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	const int32_t debugStep = input.GetInput(VK_SHIFT) ? -1 : 1;
	if (input.GetInputDown(VK_F11))
	{
		debug.rrComparisonVisible = true;
		auto& streamline = GLOBAL(StreamlineGlobal);
		if (input.GetInput(VK_CONTROL))
		{
			if (streamline.IsRayReconstructionEnabled() && !debug.BypassDlss() &&
				streamline.GetRayReconstructionEvaluatedFrames() >= 64)
			{
				++debug.rrCaptureRequest;
				debug.SetCaptureStatus(L"RR SNAPSHOT REQUESTED", 0, 1);
			}
			else
				debug.SetCaptureStatus(L"RR: SELECT BEAUTY, WAIT 64 FRAMES", 0, 1);
		}
		else if (input.GetInput(VK_SHIFT))
		{
			GLOBAL(DxRendererGlobal).ToggleDebugCameraFreeze();
			streamline.RequestHistoryReset();
		}
		else
		{
			streamline.SetRayReconstructionPresetE(!streamline.IsRayReconstructionPresetE());
		}
		debug.TouchOverlay();
	}
	if (input.GetInputDown(VK_F9))
	{
		TogglePhysicalRenderingBaseline();
	}
	if (input.GetInputDown(VK_F12))
	{
		if (input.GetInput(VK_CONTROL))
		{
			if (debug.RequestHdrCapture(m_settings.restirPrimarySpp))
			{
				++m_settings.restirHistoryGeneration;
				DEBUG_LOG_FMT(
					"[ReSTIR.Capture] requested source={} spp={}\n", debug.GetSourceLogName(),
					m_settings.restirPrimarySpp
				);
			}
			else
			{
				DEBUG_LOG_FMT("[ReSTIR.Capture] Select CANDIDATE RAW or FINAL RAW, BEAUTY first.\n");
			}
		}
		else
		{
			debug.StepSource(debugStep);
		}
	}
	if (input.GetInputDown(VK_F10))
	{
		debug.StepView(debugStep);
	}

	if (previousDebugRevision != debug.GetRevision())
	{
		const bool sourceChanged =
			previousOverrideActive != debug.IsOverrideActive() || previousSource != debug.GetSource();
		if (sourceChanged)
		{
			++m_settings.restirHistoryGeneration;
		}
		GLOBAL(StreamlineGlobal).RequestHistoryReset();
		GRAPHICS_LOG_FMT(
			"[ReSTIR.Debug] override={} source={} view={} dlss={}\n", debug.IsOverrideActive(),
			debug.GetSourceLogName(), debug.GetViewLogName(), debug.BypassDlss() ? "BYPASSED" : "REQUESTED"
		);
	}

#endif
}

RaytracingPath RaytracingControls::GetPath() const
{
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	const auto& debug = GLOBAL(RestirDebugGlobal);
	if (debug.IsOverrideActive())
	{
		return debug.UsesRestirCandidate()										 ? RaytracingPath::RestirCandidate
			   : debug.GetSource() == RestirDebugSource::ReferencePathTracingRaw ? RaytracingPath::Reference
																				 : RaytracingPath::Realtime;
	}
#endif
	return m_settings.useRestirPT	   ? RaytracingPath::RestirCandidate
		   : m_settings.usePathTracing ? RaytracingPath::Reference
									   : RaytracingPath::Realtime;
}

void RaytracingControls::ApplyOutputSettings(RaytracingOutputRenderData& output) const
{
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	const auto& debug = GLOBAL(RestirDebugGlobal);
	output.bypassDlss = debug.IsOverrideActive() && debug.BypassDlss();
	output.bypassToneMap = debug.IsOverrideActive() && debug.BypassToneMap();
#else
	output.bypassToneMap = false;
#endif
}
