#include "stdafxClient.h"
#include "PathTracingDemoDoorComponent.h"
#include "OptionsMenuComponent.h"
#include "DxRendererGlobal.h"
#include "GameObject.h"
#include "RenderPass/RaytracingPreparePass.h"
#include "RenderPass/ReferencePathTracingPass.h"
#include "ResourceGlobal.h"
#include "StreamlineGlobal.h"
#include "Transform.h"
#include <algorithm>

void PathTracingDemoDoorComponent::OnStart()
{
	GetGameObject()->GetTransform().SetRotation(0.0f, -95.0f * m_openAmount, 0.0f);
}

void PathTracingDemoDoorComponent::BeginMove(bool open)
{
	m_targetOpen = open;
	m_startAmount = m_openAmount;
	m_elapsed = 0.0f;
	m_moving = true;
}

void PathTracingDemoDoorComponent::ToggleAutomatic()
{
	if (m_automatic || m_moving)
	{
		m_automatic = false;
		m_moving = false;
	}
	else
	{
		m_automatic = true;
		BeginMove(m_openAmount < 0.5f);
	}
}

void PathTracingDemoDoorComponent::ToggleOpen()
{
	m_automatic = false;
	BeginMove(m_moving ? !m_targetOpen : m_openAmount < 0.5f);
}

void PathTracingDemoDoorComponent::OnUpdate(float deltaTime)
{
	if (OptionsMenuComponent::IsOpenInActiveScene() || GLOBAL(ResourceGlobal).HasPendingLoads() ||
		GLOBAL(StreamlineGlobal).IsFeatureWarmupActive())
	{
		return;
	}
	const float step = std::clamp(deltaTime, 0.0f, 0.1f);
	if (!m_moving)
	{
		if (!m_automatic)
		{
			return;
		}
		m_holdRemaining -= step;
		if (m_holdRemaining > 0.0f)
		{
			return;
		}
		BeginMove(!m_targetOpen);
	}
	m_elapsed += step;
	const float t = std::min(m_elapsed / 2.5f, 1.0f);
	const float smooth = t * t * (3.0f - 2.0f * t);
	m_openAmount = std::lerp(m_startAmount, m_targetOpen ? 1.0f : 0.0f, smooth);
	GetGameObject()->GetTransform().SetRotation(0.0f, -95.0f * m_openAmount, 0.0f);
	if (t >= 1.0f)
	{
		m_moving = false;
		m_holdRemaining = m_targetOpen ? 4.0f : 2.0f;
	}
	// Reference PT averages a static scene. ReSTIR keeps its normal dynamic visibility history.
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
