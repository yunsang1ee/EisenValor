#include "stdafxClient.h"
#include "BattleUI.h"
#include <numbers>

void BattleUI::InitializeBattleUI()
{
	auto& uiGlobal = UIGlobal::GetInstance();

	// 3개 UI 생성 (Handle로 저장)
	m_upUIHandle = uiGlobal.AddUI<BattleUIComponent>();
	m_leftUIHandle = uiGlobal.AddUI<BattleUIComponent>();
	m_rightUIHandle = uiGlobal.AddUI<BattleUIComponent>();

	// 화면 중앙으로 초기 위치 설정
	SetCenterPositionToScreenCenter();

#ifdef _DEBUG
	RenderDebugCircle();
	RenderDebugRegions();
#endif
}

void BattleUI::OnAttach()
{
	DEBUG_LOG_FMT("[BattleUI] OnAttach called\n");

	InitializeBattleUI();
}

void BattleUI::OnUpdate(float deltaTime)
{
	// 매 프레임 마우스 입력 처리
	ProcessMouseInput();
}

void BattleUI::OnDetach()
{
	DEBUG_LOG_FMT("[BattleUISystem] OnDetach called\n");
#ifdef _DEBUG
	ClearDebugCircle();
#endif
}

void BattleUI::UpdateUISelection(int selectedRegion)
{
	auto& uiGlobal = UIGlobal::GetInstance();

	// 각 UI에 선택 상태만 알려주기
	if (auto* upUI = uiGlobal.GetUI(m_upUIHandle))
		upUI->SetSelected(selectedRegion == 0);

	if (auto* leftUI = uiGlobal.GetUI(m_leftUIHandle))
		leftUI->SetSelected(selectedRegion == 1);

	if (auto* rightUI = uiGlobal.GetUI(m_rightUIHandle))
		rightUI->SetSelected(selectedRegion == 2);
}

void BattleUI::ProcessMouseInput()
{
	auto&		 input = GLOBAL(InputGlobal);
	DX::XMFLOAT2 mousePos = input.GetMousePosition();

	// 1. 화면 중앙 기준으로 상대 좌표 계산
	float relativeX = mousePos.x - m_centerX;
	float relativeY = mousePos.y - m_centerY;

	// 2. 거리 계산 (원 안에 있는지)
	float distanceSq = relativeX * relativeX + relativeY * relativeY;
	float radiusSq = m_radius * m_radius;

	if (distanceSq <= radiusSq)
	{
		// 3. 각도 계산 (라디안)
		float angle = atan2f(-relativeY, relativeX); // Y축 반전

		// 각도를 0 ~ 2π 범위로 변환
		if (angle < 0.0f)
			angle += TWO_PI;

		// 각도를 degree로 변환
		float angleDeg = angle * RAD_TO_DEG;

		// 4. 영역 구분
		int regionIndex = GetRegionIndex(angleDeg);

		// 5. UI 상태 업데이트
		UpdateUISelection(regionIndex);

		// 6. 마우스 클릭 처리
		if (input.GetInputDown(VK_LBUTTON))
		{
			OnRegionClicked(regionIndex);
		}
	}
	else
	{
		// 원 밖에 있으면 선택 해제
		UpdateUISelection(-1);
	}
}


void BattleUI::SetCenterPositionToScreenCenter()
{
	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
	if (swapChain)
	{
		float centerX = swapChain->GetWidth() / 2.0f;
		float centerY = swapChain->GetHeight() / 2.0f;
		SetCenterPosition(centerX, centerY);
	}
}

void BattleUI::SetCenterPosition(float x, float y)
{
	m_centerX = x;
	m_centerY = y;

	auto& uiGlobal = UIGlobal::GetInstance();

	// 원 내부에 배치 (반지름의 70% 위치)
	const float innerRadius = m_radius * INNER_RADIUS_RATIO;

	// UP: 0도 (위쪽)
	const float upX = x;
	const float upY = y - innerRadius;

	// LEFT: 120도 (왼쪽 아래)
	const float leftX = x - innerRadius * COS_30_DEG;
	const float leftY = y + innerRadius * SIN_30_DEG;

	// RIGHT: 240도 (오른쪽 아래)
	const float rightX = x + innerRadius * COS_30_DEG;
	const float rightY = y + innerRadius * SIN_30_DEG;

	if (auto* upUI = uiGlobal.GetUI(m_upUIHandle))
	{
		upUI->SetPosition(upX - UI_HALF_SIZE, upY - UI_HALF_SIZE);
		upUI->SetSize(UI_SIZE, UI_SIZE);						
	}

	if (auto* leftUI = uiGlobal.GetUI(m_leftUIHandle))
	{
		leftUI->SetPosition(leftX - UI_HALF_SIZE, leftY - UI_HALF_SIZE);
		leftUI->SetSize(UI_SIZE, UI_SIZE);
	}

	if (auto* rightUI = uiGlobal.GetUI(m_rightUIHandle))
	{
		rightUI->SetPosition(rightX - UI_HALF_SIZE, rightY - UI_HALF_SIZE);
		rightUI->SetSize(UI_SIZE, UI_SIZE);
	}
}

int BattleUI::GetRegionIndex(float angleDegrees) const
{
	NormalizeAngle(angleDegrees);

	if (angleDegrees >= UP_REGION_START && angleDegrees < UP_REGION_END) 
	{
		return 0; // UP
	}
	else if (angleDegrees >= LEFT_REGION_START && angleDegrees < LEFT_REGION_END)
	{
		return 1; // LEFT
	}
	else if (angleDegrees >= RIGHT_REGION_START || angleDegrees < RIGHT_REGION_END)
	{
		return 2; // RIGHT
	}

	return -1; // 경계선
}

void BattleUI::OnRegionClicked(int regionIndex)
{
	switch (regionIndex)
	{
	case 0: // UP
		DEBUG_LOG_FMT("[BattleUI] UP region clicked!\n");
		break;

	case 1: // LEFT
		DEBUG_LOG_FMT("[BattleUI] LEFT region clicked!\n");
		break;

	case 2: // RIGHT
		DEBUG_LOG_FMT("[BattleUI] RIGHT region clicked!\n");
		break;

	default:
		DEBUG_LOG_FMT("[BattleUI] Invalid region clicked: {}\n", regionIndex);
		break;
	}
}

#ifdef _DEBUG
void BattleUI::RenderDebugCircle()
{
	auto&	  uiGlobal = UIGlobal::GetInstance();
	const int numPoints = 32;

	for (int i = 0; i < numPoints; i++)
	{
		// 각도 계산 (0 ~ 2π, 라디안)
		float angleRad = (i / float(numPoints)) * 2.0f * 3.14159f;
		float angleDeg = (i / float(numPoints)) * 360.0f; // 각도로 변환

		// 원 둘레 좌표 계산
		float x = m_centerX + cos(angleRad) * m_radius;
		float y = m_centerY - sin(angleRad) * m_radius;

		// 작은 사각형 UI 생성 (5x5 픽셀)
		auto handle = uiGlobal.AddUI<BattleUIComponent>();
		m_debugCircleHandles.push_back(handle);

		if (auto* dot = uiGlobal.GetUI(handle))
		{
			dot->SetPosition(x - 2.5f, y - 2.5f);
			dot->SetSize(5.0f, 5.0f);

			// 영역에 따라 색상 설정
			int region = GetRegionIndex(angleDeg);
			if (region == 0)
			{											 // UP
				dot->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 빨간색
			}
			else if (region == 1)
			{											 // LEFT
				dot->SetColor({0.0f, 0.0f, 1.0f, 1.0f}); // 파란색
			}
			else if (region == 2)
			{											 // RIGHT
				dot->SetColor({1.0f, 1.0f, 0.0f, 1.0f}); // 노란색
			}
			else
			{
				dot->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // 초록색 (경계선)
			}
		}
	}
}

void BattleUI::RenderDebugRegions()
{
	auto& uiGlobal = UIGlobal::GetInstance();

	// 경계선 각도
	float boundaryAngles[] = {30.0f, 150.0f, 270.0f, 390.0f};

	// 각 경계선에 선 그리기
	for (float angleDeg : boundaryAngles)
	{
		// 각도를 라디안으로 변환
		float angleRad = angleDeg * 3.14159f / 180.0f;

		// 경계선을 여러 점으로 그리기
		for (int i = 0; i <= 20; i++)
		{
			float t = i / 20.0f;	   // 0.0 ~ 1.0
			float dist = t * m_radius; // 중심에서 거리

			float x = m_centerX + cos(angleRad) * dist;
			float y = m_centerY - sin(angleRad) * dist;

			auto handle = uiGlobal.AddUI<BattleUIComponent>();
			m_debugRegionHandles.push_back(handle);

			if (auto* dot = uiGlobal.GetUI(handle))
			{
				dot->SetPosition(x - 1.5f, y - 1.5f);
				dot->SetSize(3.0f, 3.0f);
				dot->SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // 흰색
			}
		}
	}
}
void BattleUI::ClearDebugCircle()
{
	// Handle만 비우기
	m_debugCircleHandles.clear();
	m_debugRegionHandles.clear();
}
#endif