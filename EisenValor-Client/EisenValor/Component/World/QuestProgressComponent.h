#pragma once

#include "IComponent.h"
#include "Transform.h"
#include <string>

class QuestProgressComponent final : public ComponentBase<QuestProgressComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "QuestProgressComponent"; }

	void OnUpdate(float deltaTime);
	void SetOccupationZoneReached(bool reached) { m_occupationZoneReached = reached; }

private:
	void AdvanceTo(int nextStage, const std::wstring& message);
	float GetMovedDistanceSq(const DX::XMFLOAT3& currentPosition) const;

	int m_stage = 0;
	bool m_hasStartPosition = false;
	bool m_occupationZoneReached = false;
	DX::XMFLOAT3 m_startPosition{};
};
