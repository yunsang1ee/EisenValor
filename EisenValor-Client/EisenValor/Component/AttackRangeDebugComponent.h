#pragma once
#include "IComponent.h"

class AttackRangeDebugComponent : public ComponentBase<AttackRangeDebugComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "AttackRangeDebugComponent"; }

	void OnAttach();
	void OnUpdate(float deltaTime);

	void SetRadius(float radius);
	void SetCenterAngleDegrees(float centerAngleDegrees);

private:
	void ApplyTransform();
	void LogValues() const;

	float m_radius = 1.0f;
	float m_centerAngleDegrees = 10.0f;
};
