#pragma once

#include <Singleton.h>

#include <cstdint>

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
	Count,
};

class RestirDebugGlobal final : public Singleton<RestirDebugGlobal>
{
private:
	friend class Singleton<RestirDebugGlobal>;

	RestirDebugGlobal() = default;
	~RestirDebugGlobal() override = default;

public:
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
		default:
			return "UNKNOWN";
		}
	}

private:
	RestirDebugSource m_source = RestirDebugSource::FinalWithRayReconstruction;
	RestirDebugView	  m_view = RestirDebugView::Beauty;
	uint64_t		  m_revision = 0;
	bool			  m_overrideActive = false;
};
