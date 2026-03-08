#include "stdafxClient.h"
#include "HealthComponent.h"
#include "GameObject.h"
#include "FSM/FSMComponent.h"
#include <Packets/Enums_generated.h>

void HealthComponent::SetHealth(int health)
{
	int prevHealth = m_health;
	m_health = std::clamp(health, 0, m_maxHealth);

	if (m_health <= 0) 
	{
		// FSM 상태를 DEAD로 변경
		if (auto* fsm = GetGameObject()->GetComponent<FSMComponent>())
		{
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_DEAD);
		}

		// 바로 비활성화하면 애니메이션이 보이지 않음(일단 주석처리)
		// GetGameObject()->SetActive(false);
	}
	else if (prevHealth <= 0 && m_health > 0)
	{
		// 체력이 다시 생기면 FSM을 IDLE로 복구
		if (auto* fsm = GetGameObject()->GetComponent<FSMComponent>())
		{
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
		}
	}
}