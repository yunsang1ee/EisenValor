#include "stdafxClientFramework.h"
#include "LocalPlayer.h"
#include "GlobalInterfaces.h"

using namespace DirectX;

void LocalPlayer::Update(float deltaTime)
{
#ifdef DEAD_RECKONING
	UniformAcceleration(deltaTime);
#else
	// UniformVelocity(deltaTime);
	UpdateInput(deltaTime);
#endif // DEAD_RECKONING
}

void LocalPlayer::UniformVelocity(const float deltaTime)
{
	// CS_MOVE_START
	// - 키를 처음 눌렀거나 뗏을때 || 마우스를 움직이고 있을 때
	// - 보내줘야 하는 내용
	//	  - Vec3 pos
	//    - Vec3 velocity(속력 + 방향)
	//	  - Vec3 acceleration
	//	  - Vec3 rot

	// 서버는 pos += dir * speed * server DT
	//		  m_rot.y += DT * mouse_move;

	// SC_MOVE
	// - Vec3 serverPos;
	// - Vec3 serverVelocity;
	// - Vec3 serverRot;
	// -


	// 마우스 휠로 줌인아웃
	int wheelDelta = Globals::Input().GetWheelScroll();
	if (wheelDelta != 0)
	{
		m_cameraDistance -= wheelDelta * 0.001f;
		m_cameraDistance = std::clamp(m_cameraDistance, 5.0f, 30.0f);
	}

	// 플레이어 정면 벡터
	const float forwardX = sinf(m_cameraYaw);
	const float forwardZ = cosf(m_cameraYaw);

	// 플레이어 우측 벡터
	const float rightX = sinf(m_cameraYaw + XM_PIDIV2);
	const float rightZ = cosf(m_cameraYaw + XM_PIDIV2);

	const float moveSpeed = m_playerSpeed * deltaTime;


	if (Globals::Input().GetInputUp('W') || Globals::Input().GetInputUp('S') || Globals::Input().GetInputUp('A') ||
		Globals::Input().GetInputUp('D'))
	{
	}

	if (Globals::Input().GetInput('W')) // 전진
	{
		m_pos.x += forwardX * moveSpeed;
		m_pos.z += forwardZ * moveSpeed;
		// sendFlag = true;
	}
	else if (Globals::Input().GetInput('S')) // 후진
	{
		m_pos.x -= forwardX * moveSpeed;
		m_pos.z -= forwardZ * moveSpeed;
		// sendFlag = true;
	}
	else if (Globals::Input().GetInput('A')) // 좌측 이동
	{
		m_pos.x -= rightX * moveSpeed;
		m_pos.z -= rightZ * moveSpeed;
		//	sendFlag = true;
	}
	else if (Globals::Input().GetInput('D')) // 우측 이동
	{
		m_pos.x += rightX * moveSpeed;
		m_pos.z += rightZ * moveSpeed;
		//	sendFlag = true;
	}

	//// 수직 이동 (H/L 키)
	if (Globals::Input().GetInput('H'))
	{
		m_pos.y -= moveSpeed; // 아래로
							  //		sendFlag = true;
	}
	if (Globals::Input().GetInput('L'))
	{
		m_pos.y += moveSpeed; // 위로
							  //		sendFlag = true;
	}


	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	if (m_pos.x != lastX || m_pos.z != lastZ)
	{
		DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n", m_pos.x, m_pos.y, m_pos.z);
		lastX = m_pos.x;
		lastY = m_pos.y;
		lastZ = m_pos.z;
	}

	if (sendFlag)
	{
		const Vec3	 pos{GetPosition()};
		const Vec3	 rot{0.f, m_rot.y, 0.f};
		const Vec3	 vel{GetVelocity()};
		const Vec3	 accel{GetAcceleration()};
		const uint64 timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
									 std::chrono::high_resolution_clock::now().time_since_epoch()
		)
									 .count();

		auto pb = NetBridge::ServerPacketHandler::Make_CS_MOVE_PACKET(true, true, pos, rot, vel, accel, timeStamp);
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
		sendFlag = false;
	}
}

void LocalPlayer::UniformAcceleration(const float deltaTime)
{
	m_acceleration = Vec3{0, 0, 0}; // 매 프레임 초기화
	Vec3 forwardDir{sinf(m_cameraYaw), 0.f, cosf(m_cameraYaw)};
	Vec3 rightDir{sinf(m_cameraYaw + XM_PIDIV2), 0.f, cosf(m_cameraYaw + XM_PIDIV2)};

	constexpr float accelValue = 5.f;

	if (Globals::Input().GetInputDown('W'))
	{
		m_acceleration.x += forwardDir.x * accelValue;
		m_acceleration.y += forwardDir.y * accelValue;
		m_acceleration.z += forwardDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}

	else if (Globals::Input().GetInputDown('S'))
	{
		m_acceleration.x -= forwardDir.x * accelValue;
		m_acceleration.y -= forwardDir.y * accelValue;
		m_acceleration.z -= forwardDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}
	else if (Globals::Input().GetInputUp('W'))
	{
		sendFlag = false;
	}
	else if (Globals::Input().GetInputUp('S'))
	{
		sendFlag = false;
	}
	else if (Globals::Input().GetInputUp('A'))
	{
		sendFlag = false;
	}
	else if (Globals::Input().GetInputUp('D'))
	{
		sendFlag = false;
	}
	else if (Globals::Input().GetInputDown('A'))
	{
		m_acceleration.x -= rightDir.x * accelValue;
		m_acceleration.y -= rightDir.y * accelValue;
		m_acceleration.z -= rightDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}
	else if (Globals::Input().GetInputDown('D'))
	{
		m_acceleration.x += rightDir.x * accelValue;
		m_acceleration.y += rightDir.y * accelValue;
		m_acceleration.z += rightDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}


	else if (Globals::Input().GetInput('W'))
	{
		m_acceleration.x += forwardDir.x * accelValue;
		m_acceleration.y += forwardDir.y * accelValue;
		m_acceleration.z += forwardDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		// sendFlag = true;
	}

	else if (Globals::Input().GetInput('S'))
	{
		m_acceleration.x -= forwardDir.x * accelValue;
		m_acceleration.y -= forwardDir.y * accelValue;
		m_acceleration.z -= forwardDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		// sendFlag = true;
	}

	else if (Globals::Input().GetInput('A'))
	{
		m_acceleration.x -= rightDir.x * accelValue;
		m_acceleration.y -= rightDir.y * accelValue;
		m_acceleration.z -= rightDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;

		// sendFlag = true;
	}

	else if (Globals::Input().GetInput('D'))
	{
		m_acceleration.x += rightDir.x * accelValue;
		m_acceleration.y += rightDir.y * accelValue;
		m_acceleration.z += rightDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;

		// sendFlag = true;
	}

	// 수직 이동도 추가 가능
	else if (Globals::Input().GetInput('L'))
	{
		m_acceleration.y += accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;

		//	sendFlag = true;
	}
	else if (Globals::Input().GetInput('H'))
	{
		m_acceleration.y -= accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;

		//		sendFlag = true;
	}
	else
	{
		const float dampingFactor = 0.95f; // 예: 0.9 ~ 0.95 정도로 시작

		m_velocity.x *= dampingFactor;
		m_velocity.y *= dampingFactor;
		m_velocity.z *= dampingFactor;
	}

	if (Globals::Input().GetInputDown('R'))
	{
		auto pb = NetBridge::ServerPacketHandler::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

	m_pos.x += m_velocity.x * deltaTime;
	m_pos.y += m_velocity.y * deltaTime;
	m_pos.z += m_velocity.z * deltaTime;

	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	if (m_pos.x != lastX || m_pos.z != lastZ)
	{
		DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n", m_pos.x, m_pos.y, m_pos.z);
		lastX = m_pos.x;
		lastY = m_pos.y;
		lastZ = m_pos.z;
	}



	if (sendFlag)
	{
		const Vec3	 pos{GetPosition()};
		const Vec3	 rot{0.f, m_rot.y, 0.f};
		const Vec3	 vel{GetVelocity()};
		const Vec3	 accel{GetAcceleration()};
		const uint64 timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
									 std::chrono::high_resolution_clock::now().time_since_epoch()
		)
									 .count();

		//auto pb = NetBridge::ServerPacketHandler::Make_CS_MOVE_PACKET(pos, rot, vel, accel, timeStamp);
		//MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
		//sendFlag = false;
	}

	if (Globals::Input().GetInputDown('R'))
	{
		auto pb = NetBridge::ServerPacketHandler::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}
}

void LocalPlayer::UpdateInput(const float deltaTime)
{
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
			sendFlag = true;
		}
		else
		{
			// 움직임 감지
			const float deltaX = mousePos.x - m_lastMouseX;
			const float deltaY = mousePos.y - m_lastMouseY;

			if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f)
			{
				// 카메라 회전 업데이트
				m_cameraYaw += deltaX * m_mouseSensitivity;
				m_rot.y = m_cameraYaw;
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
			sendFlag = true;
		}
	}
	else
	{
		if (m_isMouseDragging)
		{
			// 드래그 종료
			m_isMouseDragging = false;
			DEBUG_LOG_FMT("Camera drag ended\n");
			sendFlag = true;
		}
	}
	// 플레이어 정면 벡터
	const float forwardX = sinf(m_cameraYaw);
	const float forwardZ = cosf(m_cameraYaw);

	// 플레이어 우측 벡터
	const float rightX = sinf(m_cameraYaw + XM_PIDIV2);
	const float rightZ = cosf(m_cameraYaw + XM_PIDIV2);

	// 키를 처음 눌렀을 때
	if (Globals::Input().GetInputDown('W'))
	{
		m_velocity.x = forwardX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = forwardZ * m_playerSpeed;
		sendFlag = true;
	}
	if(Globals::Input().GetInputDown('S')) {
		m_velocity.x = -forwardX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = -forwardZ * m_playerSpeed;
		sendFlag = true;
	}
	if(Globals::Input().GetInputDown('A')) {
		m_velocity.x = -rightX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = -rightZ * m_playerSpeed;
		sendFlag = true;
	}

	if(Globals::Input().GetInputDown('D')) {
		m_velocity.x = rightX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = rightZ * m_playerSpeed;
		sendFlag = true;
	}

	if(Globals::Input().GetInput('W')) {
		m_velocity.x = forwardX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = forwardZ * m_playerSpeed;
	}
	if (Globals::Input().GetInput('S')) {
		m_velocity.x = -forwardX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = -forwardZ * m_playerSpeed;
	}
	if (Globals::Input().GetInput('A')) {
		m_velocity.x = -rightX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = -rightZ * m_playerSpeed;
	}
	if (Globals::Input().GetInput('D')) {
		m_velocity.x = rightX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = rightZ * m_playerSpeed;
	}
	// 키를 뗏을 때
	if (Globals::Input().GetInputUp('W') || Globals::Input().GetInputUp('S') || Globals::Input().GetInputUp('A') ||
		Globals::Input().GetInputUp('D'))
	{
		m_velocity.x = 0.f;
		m_velocity.y = 0.f;
		m_velocity.z = 0.f;
		sendFlag = true;
	}

	if (Globals::Input().GetInputDown('R')) {
		auto pb = NetBridge::ServerPacketHandler::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

	m_pos.x += m_velocity.x * deltaTime;
	m_pos.y += m_velocity.y * deltaTime;
	m_pos.z += m_velocity.z * deltaTime;
	auto now = std::chrono::high_resolution_clock::now();
	auto elapsed = now - lastSend;
	if (elapsed >= std::chrono::milliseconds(100) || sendFlag)
	{
		lastSend = now;
		const Vec3	 pos{GetPosition()};
		const Vec3	 rot{0.f, m_rot.y, 0.f};
		const Vec3	 vel{GetVelocity()};
		const Vec3	 accel{GetAcceleration()};
		const uint64 timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
									 std::chrono::high_resolution_clock::now().time_since_epoch()
		)
									 .count();

		auto pb = NetBridge::ServerPacketHandler::Make_CS_MOVE_PACKET(true, true, pos, rot, vel, accel, timeStamp);
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
		sendFlag = false;
	}

}
