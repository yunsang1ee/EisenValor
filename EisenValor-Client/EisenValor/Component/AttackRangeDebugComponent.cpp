#include "stdafxClient.h"
#include "AttackRangeDebugComponent.h"

#include "GameObject.h"
#include "InputGlobal.h"
#include "Scene.h"
#include "Transform.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kMinRadius = 0.1f;
constexpr float kMaxRadius = 20.0f;
constexpr float kRadiusStep = 0.5f;
constexpr float kSegmentAngleDegrees = 10.0f;
constexpr float kMinCenterAngleDegrees = kSegmentAngleDegrees;
constexpr float kMaxCenterAngleDegrees = 180.0f;
constexpr float kCenterAngleStepDegrees = kSegmentAngleDegrees;

float QuantizeCenterAngle(float degrees)
{
	const float clamped = std::clamp(degrees, kMinCenterAngleDegrees, kMaxCenterAngleDegrees);
	return std::round(clamped / kSegmentAngleDegrees) * kSegmentAngleDegrees;
}

} // namespace

void AttackRangeDebugComponent::OnAttach()
{
	if (auto* owner = GetGameObject())
	{
		const auto scale = owner->GetTransform().GetScale();
		m_radius = std::clamp(scale.x, kMinRadius, kMaxRadius);
	}

	ApplyTransform();
	LogValues();
}

void AttackRangeDebugComponent::OnUpdate(float)
{
	auto& input = GLOBAL(InputGlobal);
	bool changed = false;

	if (input.GetInputDown('U'))
	{
		m_radius -= kRadiusStep;
		changed = true;
	}
	if (input.GetInputDown('I'))
	{
		m_radius += kRadiusStep;
		changed = true;
	}
	if (input.GetInputDown('J'))
	{
		m_centerAngleDegrees -= kCenterAngleStepDegrees;
		changed = true;
	}
	if (input.GetInputDown('K'))
	{
		m_centerAngleDegrees += kCenterAngleStepDegrees;
		changed = true;
	}

	if (!changed)
	{
		return;
	}

	m_radius = std::clamp(m_radius, kMinRadius, kMaxRadius);
	m_centerAngleDegrees = QuantizeCenterAngle(m_centerAngleDegrees);
	ApplyTransform();
	LogValues();
}

void AttackRangeDebugComponent::SetRadius(float radius)
{
	m_radius = std::clamp(radius, kMinRadius, kMaxRadius);
	ApplyTransform();
}

void AttackRangeDebugComponent::SetCenterAngleDegrees(float centerAngleDegrees)
{
	m_centerAngleDegrees = QuantizeCenterAngle(centerAngleDegrees);
	ApplyTransform();
}

void AttackRangeDebugComponent::ApplyTransform()
{
	if (auto* owner = GetGameObject())
	{
		auto& transform = owner->GetTransform();
		transform.SetScale(m_radius, 1.0f, m_radius);

		auto* scene = owner->GetScene();
		auto* transformStorage = scene ? scene->GetStorage<Transform>() : nullptr;
		if (!transformStorage)
		{
			return;
		}

		const auto& children = transform.GetChildren();
		if (children.empty())
		{
			return;
		}

		const int totalSegmentCount = static_cast<int>(children.size());
		const int activeSegmentCount = std::clamp(
			static_cast<int>(m_centerAngleDegrees / kSegmentAngleDegrees), 1, totalSegmentCount
		);
		const int firstActiveIndex = (totalSegmentCount - activeSegmentCount) / 2;
		const int lastActiveIndex = firstActiveIndex + activeSegmentCount;
		const float firstActiveAngle =
			-0.5f * static_cast<float>(activeSegmentCount - 1) * kSegmentAngleDegrees;

		for (int index = 0; index < totalSegmentCount; ++index)
		{
			auto* childTransform = transformStorage->Get(children[index]);
			auto* childObject = childTransform ? childTransform->GetGameObject() : nullptr;
			if (!childObject)
			{
				continue;
			}

			const bool active = firstActiveIndex <= index && index < lastActiveIndex;
			childTransform->SetScale(active ? 1.0f : 0.0f);
			if (!active)
			{
				continue;
			}

			const int activeIndex = index - firstActiveIndex;
			childTransform->SetRotation(0.0f, firstActiveAngle + activeIndex * kSegmentAngleDegrees, 0.0f);
		}
	}
}

void AttackRangeDebugComponent::LogValues() const
{
	DEBUG_LOG_FMT(
		"[AttackRangeDebug] U/I radius: {:.2f}, J/K center angle: {:.1f} deg\n", m_radius,
		m_centerAngleDegrees
	);
}
