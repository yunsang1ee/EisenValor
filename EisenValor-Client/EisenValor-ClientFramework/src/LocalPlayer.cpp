#include "stdafxClientFramework.h"
#include "LocalPlayer.h"
#include "GlobalInterfaces.h"

using namespace DirectX;

void LocalPlayer::Update(float deltaTime)
{
    // GameFramework에서 플레이어 업데이트 코드를 여기로 옮김
    // 플레이어 바라보는 방향 벡터 계산
    float forwardX = sinf(m_cameraYaw);
    float forwardZ = cosf(m_cameraYaw);

	// 우측 벡터 계산
	float rightX = sinf(m_cameraYaw + XM_PIDIV2);
	float rightZ = cosf(m_cameraYaw + XM_PIDIV2);

	float moveSpeed = m_playerSpeed * deltaTime;

	// WASD 입력 처리
	if (Globals::Input().GetInput('W')) // 전진
	{
		m_x += forwardX * moveSpeed;
		m_z += forwardZ * moveSpeed;
		sendFlag = true;
	}
	if (Globals::Input().GetInput('S')) // 후진
	{
		m_x -= forwardX * moveSpeed;
		m_z -= forwardZ * moveSpeed;
		sendFlag = true;
	}
	if (Globals::Input().GetInput('A')) // 좌측 이동
	{
		m_x -= rightX * moveSpeed;
		m_z -= rightZ * moveSpeed;
		sendFlag = true;
	}
	if (Globals::Input().GetInput('D')) // 우측 이동
	{
		m_x += rightX * moveSpeed;
		m_z += rightZ * moveSpeed;
		sendFlag = true;
	}

	// 수직 이동 (H/L 키)
	if (Globals::Input().GetInput('H'))
	{
		m_y -= moveSpeed; // 아래로
		sendFlag = true;
	}
	if (Globals::Input().GetInput('L'))
	{
		m_y += moveSpeed; // 위로
		sendFlag = true;
	}

	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	if (m_x != lastX || m_z != lastZ)
	{
		DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n", m_x, m_y, m_z);
		lastX = m_x;
		lastY = m_y;
		lastZ = m_z;
	}

	// ===== 마우스로 카메라 이동 =====
	bool isLeftButtonPressed = Globals::Input().GetInput(VK_LBUTTON);
	// 현재 마우스 위치
	auto mousePos = Globals::Input().GetMousePosition();

	if (isLeftButtonPressed)
	{
		if (!m_isMouseDragging)
		{
			m_isMouseDragging = true;
			m_lastMouseX = mousePos.x; // 시작 위치 저장
			m_lastMouseY = mousePos.y;
			DEBUG_LOG_FMT("Camera drag started at ({:.1f}, {:.1f})\n", mousePos.x, mousePos.y);
		}
		else
		{
			// 움직임 감지
			float deltaX = mousePos.x - m_lastMouseX;
			float deltaY = mousePos.y - m_lastMouseY;

			if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f)
			{
				// 카메라 회전 업데이트
				m_cameraYaw += deltaX * m_mouseSensitivity;
				m_cameraPitch += deltaY * m_mouseSensitivity;

				// Pitch 제한 (위아래 회전 제한)
				m_cameraPitch = std::clamp(m_cameraPitch, -1.5f, 1.5f);

				// 디버깅
				DEBUG_LOG_FMT(
					"Camera rotating - Delta({:.1f}, {:.1f}) Yaw: {:.2f}, Pitch: {:.2f}\n", deltaX, deltaY, m_cameraYaw,
					m_cameraPitch
				);
			}

			m_lastMouseX = mousePos.x;
			m_lastMouseY = mousePos.y;
		}
	}
	else
	{
		if (m_isMouseDragging)
		{
			// 드래그 종료
			m_isMouseDragging = false;
			DEBUG_LOG_FMT("Camera drag ended\n");
		}
	}

	// 마우스 휠로 줌인아웃
	int wheelDelta = Globals::Input().GetWheelScroll();
	if (wheelDelta != 0)
	{
		m_cameraDistance -= wheelDelta * 0.001f;
		m_cameraDistance = std::clamp(m_cameraDistance, 5.0f, 30.0f);
	}

	if (sendFlag)
	{
		const FB_STRUCTS::Vec3 pos{m_x, m_y, m_z};
		const FB_STRUCTS::Vec3 rot{0.f, m_yaw, 0.f};
		const auto			   packetData = NetBridge::ServerPacketHandler::Make_CS_PLAYER_MOVE_PACKET(&pos, &rot);
		const auto			   packetBuffer =
			NetBridge::ServerPacketHandler::MakeSendBuffer(PACKET_TYPE::CS_PLAYER_MOVE, packetData);
		MANAGER(NetBridge::NetworkManager)->Send(packetBuffer);
		sendFlag = false;
	}
}