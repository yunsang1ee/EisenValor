#pragma once

#include "IComponent.h"
#include "Transform.h"
#include <string>

class QuestProgressComponent final : public ComponentBase<QuestProgressComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "QuestProgressComponent"; }

	void OnUpdate(float deltaTime);

private:
	void AdvanceTo(int nextStage, const std::wstring& message);
	float GetMovedDistanceSq(const DX::XMFLOAT3& currentPosition) const;

	int m_stage = 0;
	bool m_hasStartPosition = false;
	DX::XMFLOAT3 m_startPosition{};
};
