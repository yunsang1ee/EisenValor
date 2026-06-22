#pragma once

#include "IComponent.h"
#include <string>

class QuestUIComponent final : public ComponentBase<QuestUIComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "QuestUIComponent"; }

	void OnUpdate(float deltaTime);

	void SetMessage(const std::wstring& message);
	void RestartIntroAnimation();

private:
	float m_elapsed = 0.0f;
};
