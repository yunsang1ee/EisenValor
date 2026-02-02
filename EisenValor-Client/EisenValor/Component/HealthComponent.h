#pragma once
#include <IComponent.h>

class HealthComponent : public ComponentBase<HealthComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "HealthComponent"; }
	HealthComponent() = default;

	void SetMaxHealth(int health) { m_maxHealth = health; }
	void SetHealth(int health);
	int GetHealth() const { return m_health; }
	int GetMaxHealth() const { return m_maxHealth; }

private:
	int m_maxHealth{100};
	int m_health{100};
};
