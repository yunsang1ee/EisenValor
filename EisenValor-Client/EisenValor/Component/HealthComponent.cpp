#include "stdafxClient.h"
#include "HealthComponent.h"
#include "GameObject.h"

void HealthComponent::SetHealth(int health)
{
	m_health = std::clamp(health, 0, m_maxHealth);

	if (health <= 0) 
	{
		// TODO: 죽음 처리 (죽는 애니메이션 -> 비활성화)
		GetGameObject()->SetActive(false);
	}
}