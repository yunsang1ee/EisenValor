#include "stdafxClient.h"
#include "PathTracingDemoLightComponent.h"
#include "OptionsMenuComponent.h"
#include "DxRendererGlobal.h"
#include "GameObject.h"
#include "MaterialResource.h"
#include "RenderPass/RaytracingPreparePass.h"
#include "RenderPass/ReferencePathTracingPass.h"
#include "ResourceGlobal.h"
#include "StreamlineGlobal.h"
#include "Transform.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr float			 kColorCycleSeconds = 20.0f;
constexpr float			 kTransportIntensity = 24.0f;
constexpr float			 kVisibleIntensity = 1.0f;
constexpr float			 kOrbitSpeeds[] = {0.5f, 1.0f, 2.0f};
constexpr const wchar_t* kOrbitSpeedNames[] = {L"0.5x", L"1x", L"2x"};
constexpr const wchar_t* kColorNames[] = {L"WHITE", L"FIXED RGB", L"RAINBOW"};
constexpr const wchar_t* kComparisonHints[] = {
	L"White lights: compare moving shadow edges and reflections. L changes colors; O changes speed.",
	L"Fixed RGB: track moving colored shadows with each light's color held steady. O changes speed.",
	L"Rainbow: colors blend over 20 seconds. Compare color response at different orbit speeds with O."
};
constexpr DX::XMFLOAT3 kRainbow[] = {{1, 0.04f, 0.04f}, {1, 1, 0.04f},	   {0.04f, 1, 0.04f},
									 {0.04f, 1, 1},		{0.04f, 0.04f, 1}, {1, 0.04f, 1}};
} // namespace

void PathTracingDemoLightComponent::Configure(
	std::shared_ptr<MaterialResource> material, const DirectX::XMFLOAT3& center, float phaseOffset
)
{
	m_material = std::move(material);
	m_center = center;
	m_phaseOffset = phaseOffset - std::floor(phaseOffset);
	m_orbitTime = 12.0f * m_phaseOffset;
	m_colorTime = kColorCycleSeconds * m_phaseOffset;
}

void PathTracingDemoLightComponent::OnStart()
{
	ApplyLight();
}

const wchar_t* PathTracingDemoLightComponent::GetColorLabel() const
{
	return kColorNames[m_colorMode];
}

const wchar_t* PathTracingDemoLightComponent::GetComparisonHint() const
{
	return kComparisonHints[m_colorMode];
}

const wchar_t* PathTracingDemoLightComponent::GetOrbitSpeedLabel() const
{
	return kOrbitSpeedNames[m_orbitSpeedIndex];
}

void PathTracingDemoLightComponent::CycleOrbitSpeed()
{
	m_orbitSpeedIndex = (m_orbitSpeedIndex + 1) % static_cast<uint32_t>(std::size(kOrbitSpeeds));
}

void PathTracingDemoLightComponent::CycleColor()
{
	m_colorMode = (m_colorMode + 1) % static_cast<uint32_t>(std::size(kColorNames));
	m_colorTime = kColorCycleSeconds * m_phaseOffset;
	ApplyLight();
}

void PathTracingDemoLightComponent::ApplyLight()
{
	const float angle = DX::XM_2PI * m_orbitTime / 12.0f;
	GetGameObject()->GetTransform().SetPosition(
		m_center.x + 3.4f * std::cos(angle), m_center.y, m_center.z + 3.4f * std::sin(angle)
	);
	DX::XMFLOAT3 color{1, 1, 1};
	if (m_colorMode == 1)
	{
		const auto index = static_cast<size_t>(std::round(m_phaseOffset * 3.0f)) % 3;
		color = kRainbow[index * 2];
	}
	else if (m_colorMode == 2)
	{
		const float phase = m_colorTime / kColorCycleSeconds * static_cast<float>(std::size(kRainbow));
		const auto	index = static_cast<size_t>(phase) % std::size(kRainbow);
		const auto& a = kRainbow[index];
		const auto& b = kRainbow[(index + 1) % std::size(kRainbow)];
		const float t = phase - std::floor(phase);
		const float smooth = t * t * (3.0f - 2.0f * t);
		color = {std::lerp(a.x, b.x, smooth), std::lerp(a.y, b.y, smooth), std::lerp(a.z, b.z, smooth)};
	}
	if (m_material)
	{
		const float albedo[] = {1, 1, 1, 1};
		const float emission[] = {color.x, color.y, color.z};
		m_material->SetData(
			EvAsset::ShadingModel::LitPbr, 0, albedo, 1, 0, emission, kTransportIntensity, emission, kVisibleIntensity
		);
	}
}

void PathTracingDemoLightComponent::OnUpdate(float deltaTime)
{
	if (!m_animating || OptionsMenuComponent::IsOpenInActiveScene() || GLOBAL(ResourceGlobal).HasPendingLoads() ||
		GLOBAL(StreamlineGlobal).IsFeatureWarmupActive())
	{
		return;
	}
	const float step = std::clamp(deltaTime, 0.0f, 0.1f);
	if (step <= 0.0f)
	{
		return;
	}
	m_orbitTime = std::fmod(m_orbitTime + step * kOrbitSpeeds[m_orbitSpeedIndex], 12.0f);
	m_colorTime = std::fmod(m_colorTime + step, kColorCycleSeconds);
	ApplyLight();
	// Reference accumulation assumes a static scene; keep the normal realtime history policy.
	auto& renderer = GLOBAL(DxRendererGlobal);
	auto* prepare = static_cast<RaytracingPreparePass*>(renderer.GetRenderPass("RaytracingPrepare"));
	if (prepare && prepare->GetControls().GetPath() == RaytracingPath::Reference)
	{
		if (auto* reference = static_cast<ReferencePathTracingPass*>(renderer.GetRenderPass("ReferencePT")))
		{
			reference->ResetAccumulation();
		}
	}
}
