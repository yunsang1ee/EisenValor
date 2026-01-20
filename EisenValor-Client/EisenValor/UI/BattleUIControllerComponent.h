#pragma once
#include <IComponent.h>

class BattleUIControllerComponent : public ComponentBase<BattleUIControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "BattleUIControllerComponent"; }

	void OnAttach() override;
	void OnUpdate(float deltaTime);
	void OnDetach() override;
};