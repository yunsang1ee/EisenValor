#pragma once
#include <IComponent.h>

class StaminaComponent : public ComponentBase<StaminaComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "StaminaComponent"; }
	StaminaComponent() = default;

	void SetMaxStamina(int stamina) { m_maxStamina = stamina; }
	void SetStamina(int stamina) { m_stamina = stamina; }
	
	int GetMaxStamina() const { return m_maxStamina; }
	int GetStamina() const { return m_stamina; }
	float GetStaminaRatio() const { return static_cast<float>(m_stamina) / m_maxStamina; }

private:
	int m_maxStamina{100};
	int m_stamina{100};
};
