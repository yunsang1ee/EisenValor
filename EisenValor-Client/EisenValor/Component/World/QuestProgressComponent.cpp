#include "stdafxClient.h"
#include "QuestProgressComponent.h"

#include "GameObject.h"
#include "InputGlobal.h"
#include "QuestUIComponent.h"
#include "SceneGlobal.h"
#include "Component/TeamComponent.h"

namespace
{
bool s_waitUntilEnemyGoneAfterAlt = false;

bool IsEnemyVisible(GameObject* localPlayer, Scene* scene)
{
	if (!localPlayer || !scene)
	{
		return false;
	}

	auto* localTeamComp = localPlayer->GetComponent<TeamComponent>();
	if (!localTeamComp)
	{
		return false;
	}

	auto* teamStorage = scene->GetStorage<TeamComponent>();
	if (!teamStorage)
	{
		return false;
	}

	const auto localTeam = localTeamComp->GetTeamType();
	const auto localPos = localPlayer->GetTransform().GetPosition();
	auto forward = localPlayer->GetTransform().GetForward();

	forward.y = 0.0f;
	const float forwardLenSq = forward.x * forward.x + forward.z * forward.z;
	if (forwardLenSq <= 0.0001f)
	{
		return false;
	}

	const float forwardInvLen = 1.0f / std::sqrt(forwardLenSq);
	forward.x *= forwardInvLen;
	forward.z *= forwardInvLen;

	for (auto& teamComp : teamStorage->GetList())
	{
		auto* owner = teamComp.GetGameObject();
		if (!owner || owner == localPlayer || !owner->IsActiveInHierarchy())
		{
			continue;
		}

		const auto ownerTeam = teamComp.GetTeamType();
		if (ownerTeam == FB_ENUMS::TEAM_TYPE_NONE || ownerTeam == localTeam)
		{
			continue;
		}

		const auto enemyPos = owner->GetTransform().GetPosition();
		const float dx = enemyPos.x - localPos.x;
		const float dz = enemyPos.z - localPos.z;
		const float distanceSq = dx * dx + dz * dz;
		if (distanceSq > 400.0f || distanceSq <= 0.0001f)
		{
			continue;
		}

		const float invLen = 1.0f / std::sqrt(distanceSq);
		const float toEnemyX = dx * invLen;
		const float toEnemyZ = dz * invLen;
		const float dot = forward.x * toEnemyX + forward.z * toEnemyZ;

		if (dot >= 0.35f)
		{
			return true;
		}
	}

	return false;
}
}

void QuestProgressComponent::OnUpdate(float deltaTime)
{
	(void)deltaTime;

	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return;
	}

	auto* localPlayer = scene->FindGameObjectByServerID(scene->GetLocalID());
	if (!localPlayer)
	{
		return;
	}

	const auto pos = localPlayer->GetTransform().GetPosition();
	if (!m_hasStartPosition)
	{
		m_hasStartPosition = true;
		m_startPosition = pos;
	}

	auto& input = GLOBAL(InputGlobal);

	if (m_stage == 0)
	{
		if (GetMovedDistanceSq(pos) >= 20.0f)
		{
			AdvanceTo(1, L"Shift를 눌러 달리기하세요.");
		}
	}
	else if (m_stage == 1)
	{
		if (input.GetInput(VK_SHIFT))
		{
			if (GetMovedDistanceSq(pos) >= 50.0f)
			{
				AdvanceTo(2, L"마우스 왼쪽 버튼으로 약한 공격을 하세요.");
			}
		}
		else
		{
			m_hasStartPosition = false;
			m_startPosition = {};
		}
	}
	else if (m_stage == 2)
	{
		if (input.GetInputDown(VK_LBUTTON))
		{
			AdvanceTo(3, L"마우스 오른쪽 버튼으로 강한 공격을 하세요.");
		}
	}
	else if (m_stage == 3)
	{
		if (input.GetInputDown(VK_RBUTTON))
		{
			AdvanceTo(4, L"점령지를 찾아 이동하세요.");
		}
	}
	else if (m_stage == 4)
	{
		if (m_occupationZoneReached)
		{
			AdvanceTo(7, L"점령지를 지배하여 게이지를 채우세요.");
			return;
		}

		const bool enemyVisible = IsEnemyVisible(localPlayer, scene);

		if (s_waitUntilEnemyGoneAfterAlt)
		{
			if (!enemyVisible)
			{
				s_waitUntilEnemyGoneAfterAlt = false;
			}
		}
		else if (enemyVisible)
		{
			AdvanceTo(5, L"CTRL을 눌러 전투 모드로 진입하세요.");
		}
	}
	else if (m_stage == 5)
	{
		if (input.GetInputDown(VK_CONTROL))
		{
			AdvanceTo(6, L"ALT를 눌러 타겟을 지정하세요.");
		}
	}
	else if (m_stage == 6)
	{
		if (input.GetInputDown(VK_MENU))
		{
			s_waitUntilEnemyGoneAfterAlt = true;
			AdvanceTo(4, L"점령지를 찾아 이동하세요.");
		}
	}
	else if (m_stage == 7)
	{
	}
}

void QuestProgressComponent::AdvanceTo(int nextStage, const std::wstring& message)
{
	m_stage = nextStage;
	m_hasStartPosition = false;
	m_startPosition = {};

	if (auto* ui = GetGameObject()->GetComponent<QuestUIComponent>())
	{
		ui->SetMessage(message);
	}
}

float QuestProgressComponent::GetMovedDistanceSq(const DX::XMFLOAT3& currentPosition) const
{
	const float dx = currentPosition.x - m_startPosition.x;
	const float dy = currentPosition.y - m_startPosition.y;
	const float dz = currentPosition.z - m_startPosition.z;
	return dx * dx + dy * dy + dz * dz;
}
