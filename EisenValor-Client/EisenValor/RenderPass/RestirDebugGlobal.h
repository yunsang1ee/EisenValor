#pragma once

#include <Singleton.h>

#include <cstdint>
#include <string>

enum class RestirDebugSource : uint32_t
{
	FinalWithRayReconstruction = 0,
	FinalRaw,
	CandidateRaw,
	ReferencePathTracingRaw,
	Count,
};

enum class RestirDebugView : uint32_t
{
	Beauty = 0,
	ReservoirValid,
	SourceKind,
	WeightSum,
	SampleCount,
	TemporalAcceptance,
	TemporalRejectReason,
	MotionVectors,
	LinearDepth,
	NormalRoughness,
	DiffuseAlbedo,
	SpecularAlbedo,
	SpecularHitDistance,
	Count,
};

class RestirDebugGlobal final : public Singleton<RestirDebugGlobal>
{
private:
	friend class Singleton<RestirDebugGlobal>;

	RestirDebugGlobal() = default;
	~RestirDebugGlobal() override = default;

public:
	bool	 rrComparisonVisible = false;
	uint64_t rrCaptureRequest = 0;
	void	 TouchOverlay() { ++m_captureStatusRevision; }
	void	 UpdateRrOverlayState(uint32_t frames, bool enabled, bool presetE, bool cameraFixed)
	{
		const uint32_t progress = frames >= 64u ? 64u : (frames / 8u) * 8u;
		const uint32_t signature =
			progress | (uint32_t(enabled) << 8u) | (uint32_t(presetE) << 9u) | (uint32_t(cameraFixed) << 10u);
		if (signature != m_rrOverlaySignature)
		{
			m_rrOverlaySignature = signature;
			m_rrDisplayFrames = progress;
			TouchOverlay();
		}
	}
	uint32_t GetRrDisplayFrames() const { return m_rrDisplayFrames; }
	bool RequestHdrCapture(uint32_t spp)
	{
		if (!m_overrideActive || m_view != RestirDebugView::Beauty ||
			(m_source != RestirDebugSource::CandidateRaw && m_source != RestirDebugSource::FinalRaw))
		{
			SetCaptureStatus(L"SELECT RAW + BEAUTY", 0);
			return false;
		}
		++m_captureRequest;
		m_captureSpp = spp;
		SetCaptureStatus(L"STARTING", 0);
		return true;
	}
	[[nodiscard]] uint64_t GetCaptureRequest() const { return m_captureRequest; }
	[[nodiscard]] uint32_t GetCaptureSpp() const { return m_captureSpp; }
	void				   SetCaptureStatus(const wchar_t* status, uint32_t frames, uint32_t target = 512)
	{
		m_captureStatus = status;
		m_captureFrames = frames;
		m_captureTarget = target;
		++m_captureStatusRevision;
	}
	[[nodiscard]] std::wstring GetCaptureStatusText() const
	{
		return m_captureStatus + L"  " + std::to_wstring(m_captureFrames) + L" / " + std::to_wstring(m_captureTarget);
	}
	[[nodiscard]] uint64_t GetOverlayRevision() const { return m_revision + m_captureStatusRevision; }

	void StepSource(int32_t direction)
	{
		const auto count = static_cast<int32_t>(RestirDebugSource::Count);
		const auto current = static_cast<int32_t>(m_source);
		m_source = static_cast<RestirDebugSource>((current + direction + count) % count);
		m_overrideActive = true;
		if (RestirDebugSource::ReferencePathTracingRaw == m_source)
		{
			m_view = RestirDebugView::Beauty;
		}
		++m_revision;
	}

	void StepView(int32_t direction)
	{
		if (RestirDebugSource::ReferencePathTracingRaw == m_source)
		{
			m_source = RestirDebugSource::FinalWithRayReconstruction;
		}

		const auto count = static_cast<int32_t>(RestirDebugView::Count);
		const auto current = static_cast<int32_t>(m_view);
		m_view = static_cast<RestirDebugView>((current + direction + count) % count);
		m_overrideActive = true;
		++m_revision;
	}

	void DisableOverride()
	{
		if (!m_overrideActive && RestirDebugSource::FinalWithRayReconstruction == m_source &&
			RestirDebugView::Beauty == m_view)
		{
			return;
		}

		m_overrideActive = false;
		m_source = RestirDebugSource::FinalWithRayReconstruction;
		m_view = RestirDebugView::Beauty;
		++m_revision;
	}

	[[nodiscard]] bool IsOverrideActive() const { return m_overrideActive; }
	[[nodiscard]] bool UsesRestirCandidate() const { return RestirDebugSource::ReferencePathTracingRaw != m_source; }
	[[nodiscard]] bool UsesTemporalReuse() const
	{
		return RestirDebugSource::FinalWithRayReconstruction == m_source || RestirDebugSource::FinalRaw == m_source;
	}
	[[nodiscard]] bool BypassDlss() const
	{
		return RestirDebugSource::FinalWithRayReconstruction != m_source || RestirDebugView::Beauty != m_view;
	}
	[[nodiscard]] bool				BypassToneMap() const { return RestirDebugView::Beauty != m_view; }
	[[nodiscard]] RestirDebugSource GetSource() const { return m_source; }
	[[nodiscard]] RestirDebugView	GetView() const { return m_view; }
	[[nodiscard]] uint64_t			GetRevision() const { return m_revision; }

	[[nodiscard]] const wchar_t* GetSourceName() const
	{
		switch (m_source)
		{
		case RestirDebugSource::FinalWithRayReconstruction:
			return L"FINAL + RR";
		case RestirDebugSource::FinalRaw:
			return L"FINAL RAW";
		case RestirDebugSource::CandidateRaw:
			return L"CANDIDATE RAW";
		case RestirDebugSource::ReferencePathTracingRaw:
			return L"REFERENCE PT RAW";
		default:
			return L"UNKNOWN";
		}
	}

	[[nodiscard]] const char* GetSourceLogName() const
	{
		switch (m_source)
		{
		case RestirDebugSource::FinalWithRayReconstruction:
			return "FINAL+RR";
		case RestirDebugSource::FinalRaw:
			return "FINAL_RAW";
		case RestirDebugSource::CandidateRaw:
			return "CANDIDATE_RAW";
		case RestirDebugSource::ReferencePathTracingRaw:
			return "REFERENCE_PT_RAW";
		default:
			return "UNKNOWN";
		}
	}

	[[nodiscard]] const wchar_t* GetViewName() const
	{
		switch (m_view)
		{
		case RestirDebugView::Beauty:
			return L"BEAUTY";
		case RestirDebugView::ReservoirValid:
			return L"RESERVOIR VALID";
		case RestirDebugView::SourceKind:
			return L"SOURCE KIND";
		case RestirDebugView::WeightSum:
			return L"W HEATMAP";
		case RestirDebugView::SampleCount:
			return L"M HEATMAP";
		case RestirDebugView::TemporalAcceptance:
			return L"TEMPORAL ACCEPT / REJECT";
		case RestirDebugView::TemporalRejectReason:
			return L"TEMPORAL REJECT REASON";
		case RestirDebugView::MotionVectors:
			return L"MOTION VECTORS";
		case RestirDebugView::LinearDepth:
			return L"LINEAR DEPTH";
		case RestirDebugView::NormalRoughness:
			return L"NORMAL / ROUGHNESS";
		case RestirDebugView::DiffuseAlbedo:
			return L"DIFFUSE ALBEDO";
		case RestirDebugView::SpecularAlbedo:
			return L"SPECULAR ALBEDO";
		case RestirDebugView::SpecularHitDistance:
			return L"SPECULAR HIT DISTANCE";
		default:
			return L"UNKNOWN";
		}
	}

	[[nodiscard]] const char* GetViewLogName() const
	{
		switch (m_view)
		{
		case RestirDebugView::Beauty:
			return "BEAUTY";
		case RestirDebugView::ReservoirValid:
			return "RESERVOIR_VALID";
		case RestirDebugView::SourceKind:
			return "SOURCE_KIND";
		case RestirDebugView::WeightSum:
			return "W_HEATMAP";
		case RestirDebugView::SampleCount:
			return "M_HEATMAP";
		case RestirDebugView::TemporalAcceptance:
			return "TEMPORAL_ACCEPT_REJECT";
		case RestirDebugView::TemporalRejectReason:
			return "TEMPORAL_REJECT_REASON";
		case RestirDebugView::MotionVectors:
			return "MOTION_VECTORS";
		case RestirDebugView::LinearDepth:
			return "LINEAR_DEPTH";
		case RestirDebugView::NormalRoughness:
			return "NORMAL_ROUGHNESS";
		case RestirDebugView::DiffuseAlbedo:
			return "DIFFUSE_ALBEDO";
		case RestirDebugView::SpecularAlbedo:
			return "SPECULAR_ALBEDO";
		case RestirDebugView::SpecularHitDistance:
			return "SPECULAR_HIT_DISTANCE";
		default:
			return "UNKNOWN";
		}
	}

private:
	RestirDebugSource m_source = RestirDebugSource::FinalWithRayReconstruction;
	RestirDebugView	  m_view = RestirDebugView::Beauty;
	uint64_t		  m_revision = 0;
	uint64_t		  m_captureRequest = 0;
	uint32_t		  m_captureSpp = 1;
	uint64_t m_captureStatusRevision = 0;
	uint32_t		  m_rrOverlaySignature = 0xffffffffu;
	uint32_t		  m_rrDisplayFrames = 0;
	uint32_t		  m_captureTarget = 512;
	uint32_t m_captureFrames = 0;
	std::wstring m_captureStatus = L"READY";
	bool			  m_overrideActive = false;
};
