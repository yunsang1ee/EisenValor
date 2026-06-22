#include "stdafxClient.h"
#include "QuestProgressComponent.h"

#include "GameObject.h"
#include "InputGlobal.h"
#include "QuestUIComponent.h"
#include "SceneGlobal.h"

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

