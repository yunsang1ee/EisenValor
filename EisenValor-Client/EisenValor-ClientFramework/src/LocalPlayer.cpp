#include "stdafxClientFramework.h"
#include "LocalPlayer.h"
#include "GlobalInterfaces.h"
#include "Vertex.h"

using namespace DirectX;

void LocalPlayer::Initialize(ID3D12Device* device)
{
	// 부모 클래스 초기화
	Player::Initialize(device);

	// 부채꼴 초기화
	InitializeFan(device);

	// 와이어프레임 박스 초기화
	// InitializeWireFrame(device);

	// 지휘 영역 초기화
	InitializeCommandArea(device);

	 // 디버그 원 초기화
	InitializeDebugCircle(device);
}

void LocalPlayer::InitializeWireFrame(ID3D12Device* device)
{
	// 와이어프레임 박스 크기
	float halfW = 1.6f;
	float halfH = 1.0f;
	float halfD = 1.6f;

	// 8개 꼭짓점 (초록색 와이어프레임)
	Vertex vertices[] = {

		{DirectX::XMFLOAT3(-halfW, -halfH, -halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT3(-halfW, halfH, -halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT3(halfW, halfH, -halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT3(halfW, -halfH, -halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT3(-halfW, -halfH, halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT3(-halfW, halfH, halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT3(halfW, halfH, halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT3(halfW, -halfH, halfD), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}
	};

	// 버텍스 버퍼 생성
	const UINT			  vertexBufferSize = sizeof(vertices);
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = vertexBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_wireFrameVertexBuffer)
	));

	UINT8*		pVertexDataBegin;
	D3D12_RANGE readRange = {0, 0};
	ThrowIfFailed(m_wireFrameVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, vertices, sizeof(vertices));
	m_wireFrameVertexBuffer->Unmap(0, nullptr);

	m_wireFrameVertexBufferView.BufferLocation = m_wireFrameVertexBuffer->GetGPUVirtualAddress();
	m_wireFrameVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_wireFrameVertexBufferView.SizeInBytes = vertexBufferSize;

	// 와이어프레임 인덱스 (12개 모서리)
	UINT indices[] = {
		0, 1, 1, 2, 2, 3, 3, 0, // 앞면
		4, 5, 5, 6, 6, 7, 7, 4, // 뒷면
		0, 4, 1, 5, 2, 6, 3, 7	// 연결
	};

	const UINT indexBufferSize = sizeof(indices);
	bufferDesc.Width = indexBufferSize;
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_wireFrameIndexBuffer)
	));

	UINT8* pIndexDataBegin;
	ThrowIfFailed(m_wireFrameIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices, sizeof(indices));
	m_wireFrameIndexBuffer->Unmap(0, nullptr);

	m_wireFrameIndexBufferView.BufferLocation = m_wireFrameIndexBuffer->GetGPUVirtualAddress();
	m_wireFrameIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_wireFrameIndexBufferView.SizeInBytes = indexBufferSize;

	// 상수 버퍼 생성
	const UINT constantBufferSize = sizeof(ConstantBuffer);
	bufferDesc.Width = constantBufferSize;
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_wireFrameConstantBuffer)
	));

	ThrowIfFailed(m_wireFrameConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pWireFrameCbvDataBegin)));
}

void LocalPlayer::InitializeCommandArea(ID3D12Device* device)
{
	// 빨간 직사각형 버텍스 (바닥에 평평하게 놓인 사각형)
	float halfW = m_commandAreaWidth * 0.5f;
	float halfH = m_commandAreaHeight * 0.5f;

	Vertex vertices[] = {
		// 바닥에 평평한 사각형 (Y=0)
		{DirectX::XMFLOAT3(-halfW, 0.01f, -halfH), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.7f)}, // 빨간색, 약간 투명
		{DirectX::XMFLOAT3(halfW, 0.01f, -halfH), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.7f)},
		{DirectX::XMFLOAT3(halfW, 0.01f, halfH), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.7f)},
		{DirectX::XMFLOAT3(-halfW, 0.01f, halfH), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.7f)}
	};

	// 버텍스 버퍼 생성
	const UINT			  vertexBufferSize = sizeof(vertices);
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = vertexBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_commandAreaVertexBuffer)
	));

	UINT8*		pVertexDataBegin;
	D3D12_RANGE readRange = {0, 0};
	ThrowIfFailed(m_commandAreaVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, vertices, sizeof(vertices));
	m_commandAreaVertexBuffer->Unmap(0, nullptr);

	m_commandAreaVertexBufferView.BufferLocation = m_commandAreaVertexBuffer->GetGPUVirtualAddress();
	m_commandAreaVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_commandAreaVertexBufferView.SizeInBytes = vertexBufferSize;

	// 인덱스 (두 개의 삼각형으로 사각형 만들기)
	UINT indices[] = {
		0, 2, 1, // 첫 번째 삼각형
		0, 3, 2	 // 두 번째 삼각형
	};

	const UINT indexBufferSize = sizeof(indices);
	bufferDesc.Width = indexBufferSize;
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_commandAreaIndexBuffer)
	));

	UINT8* pIndexDataBegin;
	ThrowIfFailed(m_commandAreaIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices, sizeof(indices));
	m_commandAreaIndexBuffer->Unmap(0, nullptr);

	m_commandAreaIndexBufferView.BufferLocation = m_commandAreaIndexBuffer->GetGPUVirtualAddress();
	m_commandAreaIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_commandAreaIndexBufferView.SizeInBytes = indexBufferSize;

	// 상수 버퍼 생성
	const UINT constantBufferSize = sizeof(ConstantBuffer);
	bufferDesc.Width = constantBufferSize;
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_commandAreaConstantBuffer)
	));

	ThrowIfFailed(m_commandAreaConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCommandAreaCbvDataBegin))
	);
}

DirectX::XMFLOAT3 LocalPlayer::CalculateGroundTargetPosition() const
{
	// 플레이어가 바라보는 방향 벡터 계산
	float forwardX = sinf(m_cameraYaw);
	float forwardZ = cosf(m_cameraYaw);


	// 플레이어 위치에서 바라보는 방향으로 일정 거리만큼 떨어진 바닥 위치
	DirectX::XMFLOAT3 targetPos;
	Vec3			  pos = GetPosition();
	targetPos.x = pos.x + forwardX * m_commandAreaDistance;
	targetPos.y = -0.5f; // 바닥 높이
	targetPos.z = pos.z + forwardZ * m_commandAreaDistance;

	return targetPos;
}

void LocalPlayer::RenderCommandArea(
	ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection
)
{
	// 버텍스/인덱스 버퍼 설정
	cmdList->IASetVertexBuffers(0, 1, &m_commandAreaVertexBufferView);
	cmdList->IASetIndexBuffer(&m_commandAreaIndexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 지휘 영역 위치 계산
	DirectX::XMFLOAT3 targetPos = CalculateGroundTargetPosition();

	// 월드 변환 행렬 (위치만 적용, 회전은 플레이어 방향에 맞춤)
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(m_cameraYaw);
	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(targetPos.x, targetPos.y, targetPos.z);
	DirectX::XMMATRIX world = rotation * translation;

	// MVP 행렬 계산
	DirectX::XMMATRIX mvp = world * view * projection;
	DirectX::XMStoreFloat4x4(&m_commandAreaConstantBufferData.mvp, DirectX::XMMatrixTranspose(mvp));

	// 상수 버퍼 업데이트
	memcpy(m_pCommandAreaCbvDataBegin, &m_commandAreaConstantBufferData, sizeof(m_commandAreaConstantBufferData));

	// 렌더링
	cmdList->SetGraphicsRootConstantBufferView(0, m_commandAreaConstantBuffer->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0); // 6개 인덱스 (2개 삼각형)
}


void LocalPlayer::Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// 플레이어 자체 렌더링
	Player::Render(cmdList, view, projection);

	// 부채꼴 렌더링
	RenderFan(cmdList, view, projection);

	// 지휘 영역 렌더링
	RenderCommandArea(cmdList, view, projection);

	// 디버그 원 렌더링 (전투 모드일 때만)
	RenderDebugCircle(cmdList, view, projection);
}

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
		Vec3 currentPos = GetPosition();
		currentPos.x += forwardX * moveSpeed;
		currentPos.z += forwardZ * moveSpeed;
		SetPosition(currentPos);
		// sendFlag = true;
	}
	else if (Globals::Input().GetInput('S')) // 후진
	{
		Vec3 currentPos = GetPosition();
		currentPos.x -= forwardX * moveSpeed;
		currentPos.z -= forwardZ * moveSpeed;
		SetPosition(currentPos);
		// sendFlag = true;
	}
	else if (Globals::Input().GetInput('A')) // 좌측 이동
	{
		Vec3 currentPos = GetPosition();
		currentPos.x -= rightX * moveSpeed;
		currentPos.z -= rightZ * moveSpeed;
		SetPosition(currentPos);
		//	sendFlag = true;
	}
	else if (Globals::Input().GetInput('D')) // 우측 이동
	{
		Vec3 currentPos = GetPosition();
		currentPos.x += rightX * moveSpeed;
		currentPos.z += rightZ * moveSpeed;
		SetPosition(currentPos);
		//	sendFlag = true;
	}

	//// 수직 이동 (H/L 키)
	if (Globals::Input().GetInput('H'))
	{
		Vec3 currentPos = GetPosition();
		currentPos.y -= moveSpeed;
		SetPosition(currentPos); // 아래로
		 //		sendFlag = true;
	}
	if (Globals::Input().GetInput('L'))
	{
		Vec3 currentPos = GetPosition();
		currentPos.y += moveSpeed;
		SetPosition(currentPos); // 위로
							  //		sendFlag = true;
	}


	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	Vec3 pos = GetPosition();
	if (pos.x != lastX || pos.z != lastZ)
	{
		// DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n", pos.x, pos.y, pos.z);
		lastX = pos.x;
		lastY = pos.y;
		lastZ = pos.z;
	}

	if (sendFlag)
	{
		const Vec3	 pos{GetPosition()};
		Vec3 currentRot = GetRotation();
		const Vec3	 rot{0.f, currentRot.y, 0.f};
		const Vec3	 vel{GetVelocity()};
		const Vec3	 accel{GetAcceleration()};
		const uint64 timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
									 std::chrono::high_resolution_clock::now().time_since_epoch()
		)
									 .count();

		auto pb = NetBridge::ClientPackets::Make_CS_MOVE_PACKET(pos, rot, vel, accel, timeStamp);
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

	// 수직 이동
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
		auto pb = NetBridge::ClientPackets::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

    Vec3 currentPos = GetPosition();
	currentPos.x += m_velocity.x * deltaTime;
	currentPos.y += m_velocity.y * deltaTime;
	currentPos.z += m_velocity.z * deltaTime;
	SetPosition(currentPos);

	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	Vec3 pos = GetPosition();
	if (pos.x != lastX || pos.z != lastZ)
	{
		// DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n", pos.x, pos.y, pos.z);
		lastX = pos.x;
		lastY = pos.y;
		lastZ = pos.z;
	}

	if (sendFlag)
	{
		const Vec3	 pos{GetPosition()};
		Vec3		 currentRot = GetRotation();
		const Vec3	 rot{0.f, currentRot.y, 0.f};
		const Vec3	 vel{GetVelocity()};
		const Vec3	 accel{GetAcceleration()};
		const uint64 timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
									 std::chrono::high_resolution_clock::now().time_since_epoch()
		)
									 .count();

		// auto pb = NetBridge::ServerPacketHandler::Make_CS_MOVE_PACKET(pos, rot, vel, accel, timeStamp);
		// MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
		// sendFlag = false;
	}

	if (Globals::Input().GetInputDown('R'))
	{
		auto pb = NetBridge::ClientPackets::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}
}

// 컨트롤 토글
void LocalPlayer::UpdateInput(const float deltaTime)
{
	const auto& input = Globals::Input();

	// Ctrl 키로 카메라 모드 토글
	if (input.GetInputDown(VK_CONTROL))
	{
		if (m_cameraMode == CameraMode::NORMAL)
		{
			m_cameraMode = CameraMode::COMBAT;
			// 전투 모드로 전환 시 마우스 커서 보이기
			ShowCursor(TRUE);
		}
		else
		{
			m_cameraMode = CameraMode::NORMAL;
			// 일반 모드로 전환 시 마우스 커서 숨기기
			ShowCursor(FALSE);

			// UI 선택 해제
			SetUISelection(UISelection::NONE);
		}
	}

	// 카메라 모드에 따라 다른 입력 처리
	if (m_cameraMode == CameraMode::NORMAL)
	{
		UpdateNormalModeInput(deltaTime);
	}
	else
	{
		UpdateCombatModeInput(deltaTime);
	}
}

// 일반 모드 Input
void LocalPlayer::UpdateNormalModeInput(const float deltaTime)
{
	const auto& input = Globals::Input();
	const bool	isLeftButtonPressed = input.GetInput(VK_LBUTTON);

	// 현재 마우스 위치
	const auto mousePos = input.GetMousePosition();

	if (isLeftButtonPressed)
	{
		if (!m_isMouseDragging)
		{
			m_isMouseDragging = true;
			m_lastMouseX = mousePos.x; // 시작 위치 저장
			m_lastMouseY = mousePos.y;
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
				Vec3 currentRot = GetRotation();
				currentRot.y = m_cameraYaw;
				SetRotation(currentRot);
				m_cameraPitch += deltaY * m_mouseSensitivity;

				// Pitch 제한 (위아래 회전 제한)
				m_cameraPitch = std::clamp(m_cameraPitch, -1.5f, 1.5f);
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
		}
	}

	// 키보드 입력 처리
	HandleKeyboardInput(deltaTime);
}

void LocalPlayer::UpdateCombatModeInput(const float deltaTime)
{
	const auto& input = Globals::Input();

	// 마우스의 현재 절대 위치 가져오기 (화면 좌표)
	auto  mousePos = input.GetMousePosition();
	float mouseX = mousePos.x;
	float mouseY = mousePos.y;

	// 화면 중앙 좌표 계산
	if (m_hWnd != nullptr)
	{
		RECT clientRect;
		GetClientRect(m_hWnd, &clientRect);

		float centerX = (clientRect.left + clientRect.right) / 2.0f;
		float centerY = (clientRect.top + clientRect.bottom) / 2.0f;

		// 마우스 위치를 중앙 기준으로 변환
		float relativeX = mouseX - centerX;
		float relativeY = mouseY - centerY;

		// 화면 크기
		float width = static_cast<float>(clientRect.right - clientRect.left);
		float height = static_cast<float>(clientRect.bottom - clientRect.top);
		float aspectRatio = width / height;

		// NDC 좌표로 변환 (종횡비 보정 적용)
		// 화면의 작은 쪽을 기준으로 스케일
		float scale = (width < height) ? width : height;
		float ndcX = (relativeX / scale) * 2.0f * aspectRatio; // 종횡비 보정
		float ndcY = -(relativeY / scale) * 2.0f;			   // Y축 반전

		// 종횡비 보정된 NDC X 좌표
		float ndcXScaled = ndcX / aspectRatio;

		// 거리 계산 (원 안에 있는지 확인)
		float		distance = sqrtf(ndcXScaled * ndcXScaled + ndcY * ndcY);
		const float circleRadius = 0.6f; // InitializeDebugCircle에서 설정한 반지름과 동일
										 
		//////////////////////////////// 각도 계산 (항상 계산) //////////////////
		float angle = atan2f(ndcY, ndcXScaled);
		if (angle < 0.0f)
			angle += 2.0f * XM_PI;
		float angleDeg = angle * 180.0f / XM_PI;

		///////////////////// 실시간 디버그 출력 (매 프레임)
		//static int frameCount = 0;
		//frameCount++;

		//if (frameCount % 1 == 0) // 매 프레임 출력 (너무 많으면 % 10 등으로 조정)
		//{
		//	std::cout << std::format(
		//		"Angle: {:.1f}° | Inside: {}\n", angleDeg, (distance <= circleRadius ? "YES" : "NO")
		//	);
		//}
		///////////////////////////////////////////////////////////////////////
		static UISelection lastSelection = UISelection::NONE;
		
		// 원 안에 마우스가 있는지 확인
		if (distance <= circleRadius)
		{
			// 각도 계산 (라디안)
			float angle = atan2f(ndcY, ndcX);

			// 각도를 0 ~ 2π 범위로 변환
			if (angle < 0.0f)
				angle += 2.0f * XM_PI;

			// 각도를 도(degree)로 변환
			float angleDeg = angle * 180.0f / XM_PI;

			// 원을 삼등분
			const float PI_OVER_6 = XM_PI / 6.0f;			   // 30도
			const float FIVE_PI_OVER_6 = 5.0f * XM_PI / 6.0f;  // 150도
			const float THREE_PI_OVER_2 = 3.0f * XM_PI / 2.0f; // 270도

			UISelection newSelection = UISelection::NONE;

			if (angle >= PI_OVER_6 && angle < FIVE_PI_OVER_6)
			{
				// 위쪽 영역 (30도 ~ 150도) -> UP
				newSelection = UISelection::UP;
			}
			else if (angle >= FIVE_PI_OVER_6 && angle < THREE_PI_OVER_2)
			{
				// 왼쪽 영역 (150도 ~ 270도) -> LEFT
				newSelection = UISelection::LEFT;
			}
			else
			{
				// 오른쪽 영역 (270도 ~ 30도) -> RIGHT
				newSelection = UISelection::RIGHT;
			}

			// UI 선택 상태가 변경되었을 때만 처리
			
			if (newSelection != lastSelection)
			{
				SetUISelection(newSelection);

				const char* selectionName = "";
				switch (newSelection)
				{
				case UISelection::UP:
					selectionName = "UP";
					break;
				case UISelection::LEFT:
					selectionName = "LEFT";
					break;
				case UISelection::RIGHT:
					selectionName = "RIGHT";
					break;
				case UISelection::NONE:
					selectionName = "NONE";
					break;
				}

				std::cout << std::format(
					"UI Selection: {} | Angle: {:.1f}° | Mouse: ({:.1f}, {:.1f}) | NDC: ({:.3f}, {:.3f}) | Distance: "
					"{:.3f}\n",
					selectionName, angleDeg, mouseX, mouseY, ndcX, ndcY, distance
				);

				lastSelection = newSelection;
			}
		}
		else
		{
			// 원 밖에 있으면 선택 해제
			if (lastSelection != UISelection::NONE)
			{
				SetUISelection(UISelection::NONE);
				std::cout << "UI Selection: NONE (mouse outside circle)\n";
				lastSelection = UISelection::NONE;
			}
		}
	}

	// 키보드 입력 처리
	HandleKeyboardInput(deltaTime);

	// 전투 모드 전용 카메라 각도 설정
	Vec3 currentRot = GetRotation();
	m_cameraYaw = currentRot.y;
	m_cameraPitch = -0.3f;
}

void LocalPlayer::HandleKeyboardInput(const float deltaTime)
{
	const auto& input = Globals::Input();

	// 플레이어 정면 벡터
	const float forwardX = sinf(m_cameraYaw);
	const float forwardZ = cosf(m_cameraYaw);

	// 플레이어 우측 벡터
	const float rightX = sinf(m_cameraYaw + XM_PIDIV2);
	const float rightZ = cosf(m_cameraYaw + XM_PIDIV2);
	if (input.GetInputDown('E'))
	{
		m_playerSpeed += 10.f;
	}

	if (input.GetInputUp('E'))
	{
		m_playerSpeed -= 10.f;
	}

	if (input.GetInputDown('U'))
	{
		auto pb = NetBridge::ClientPackets::Make_CS_CHANGE_SOLDIER_FORMATION();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

	if (input.GetInput('W'))
	{
		m_velocity.x = forwardX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = forwardZ * m_playerSpeed;
	}
	if (input.GetInput('S'))
	{
		m_velocity.x = -forwardX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = -forwardZ * m_playerSpeed;
	}
	if (input.GetInput('A'))
	{
		m_velocity.x = -rightX * m_playerSpeed;
		m_velocity.y = 0.f;
		m_velocity.z = -rightZ * m_playerSpeed;
	}
	if (input.GetInput('D'))
	{
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


	if (Globals::Input().GetInputDown('R'))
	{
		auto pb = NetBridge::ClientPackets::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

	if (Globals::Input().GetInputDown('Q'))
	{
		auto pb = NetBridge::ClientPackets::Make_CS_PLAYER_ATTACK_PACKET();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

	if (Globals::Input().GetInputDown('T'))
	{
		// auto pos = GetPosition();

		//// 플레이어가 바라보는 방향 벡터 (XZ 평면)
		// const float forwardX = sinf(m_cameraYaw);
		// const float forwardZ = cosf(m_cameraYaw);

		//// 5.f 만큼 떨어진 위치
		// pos.x += forwardX * 5.f;
		// pos.z += forwardZ * 5.f;
		Vec3 pos;
		auto pb = NetBridge::ClientPackets::Make_CS_SOLDIER_MOVE_PACKET(pos);
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

	if (Globals::Input().GetInputDown('K'))
	{
		auto pb = NetBridge::ClientPackets::Make_CS_REQ_ATTACK();
		MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	}

	// if(input.GetInputDown(VK_F1)) {
	//	auto pb = NetBridge::ServerPacketHandler::Make_CS_SOLDIER_FORMATION(SOLDIER_FORMATION::FORMATION_1);
	//	MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	// }
	// else if(input.GetInputDown(VK_F2)) {
	//	auto pb = NetBridge::ServerPacketHandler::Make_CS_SOLDIER_FORMATION(SOLDIER_FORMATION::FORMATION_2);
	//	MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	// }
	// else if (input.GetInputDown(VK_F3))
	//{
	//	auto pb = NetBridge::ServerPacketHandler::Make_CS_SOLDIER_FORMATION(SOLDIER_FORMATION::FORMATION_3);
	//	MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	// }

	UpdatePos(deltaTime);

	const auto now = std::chrono::high_resolution_clock::now();
	const auto elapsed = now - lastSend;
	if (sendFlag || elapsed >= std::chrono::milliseconds(100))
	{
		SendMovePacket();
		lastSend = now;
	}
}

// virtual - override
DirectX::XMMATRIX LocalPlayer::GetViewMatrix() const
{
	if (m_cameraMode == CameraMode::NORMAL)
	{
		// 기존 방식
		return Player::GetViewMatrix();
	}
	else
	{
		Vec3  pos = GetPosition();
		Vec3  rot = GetRotation();
		float camX = pos.x - m_combatCameraDistance * sinf(rot.y);
		float camY = pos.y + m_combatCameraHeight;
		float camZ = pos.z - m_combatCameraDistance * cosf(rot.y);

		float lookX = pos.x + sinf(rot.y) * 10.0f;
		float lookY = pos.y + 1.0f;
		float lookZ = pos.z + cosf(rot.y) * 10.0f;

		return DirectX::XMMatrixLookAtLH(
			DirectX::XMVectorSet(camX, camY, camZ, 0.0f), DirectX::XMVectorSet(lookX, lookY, lookZ, 0.0f),
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
		);
	}
}

void LocalPlayer::UpdatePos(const float deltaTime)
{
	Vec3 currentPos = GetPosition();
	currentPos.x += m_velocity.x * deltaTime;
	currentPos.y += m_velocity.y * deltaTime;
	currentPos.z += m_velocity.z * deltaTime;
	SetPosition(currentPos);
}

void LocalPlayer::SendMovePacket()
{
	const Vec3	 pos{GetPosition()};
	const Vec3	 rot{GetRotation()};
	const Vec3	 vel{GetVelocity()};
	const Vec3	 accel{GetAcceleration()};
	const uint64 timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
								 std::chrono::high_resolution_clock::now().time_since_epoch()
	)
								 .count();

	auto pb = NetBridge::ClientPackets::Make_CS_MOVE_PACKET(pos, rot, vel, accel, timeStamp);
	MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));
	sendFlag = false;
}

void LocalPlayer::InitializeFan(ID3D12Device* device)
{
	// 부채꼴 정점 데이터 생성
	std::vector<Vertex> fanVertices;
	std::vector<UINT>	fanIndices;

	// 중심점 (부채꼴의 꼭짓점)
	fanVertices.push_back({XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)}); // 노란색

	// 부채꼴 호의 정점들 생성
	for (int i = 0; i <= m_fanSegments; i++)
	{
		// -m_fanAngle/2 부터 +m_fanAngle/2 까지의 각도 계산
		float theta = -m_fanAngle * 0.5f + (m_fanAngle * i) / m_fanSegments;

		// X-Z 평면에서 부채꼴 생성 (Y는 0.01f로 살짝 위에)
		float x = m_fanRadius * sinf(theta);
		float z = m_fanRadius * cosf(theta);

		fanVertices.push_back({XMFLOAT3(x, 0.01f, z), XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f)}); // 주황색
	}

	// 인덱스 생성 (삼각형들로 부채꼴 구성)
	for (int i = 0; i < m_fanSegments; i++)
	{
		fanIndices.push_back(0);	 // 중심점
		fanIndices.push_back(i + 1); // 현재 호 점
		fanIndices.push_back(i + 2); // 다음 호 점
	}

	m_fanIndexCount = static_cast<UINT>(fanIndices.size());

	// 힙 속성 설정
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RANGE readRange = {0, 0};

	// 부채꼴 정점 버퍼 생성
	const UINT			fanVertexBufferSize = static_cast<UINT>(fanVertices.size() * sizeof(Vertex));
	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = fanVertexBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_fanVertexBuffer)
	));

	// 부채꼴 정점 데이터 복사
	UINT8* pFanVertexDataBegin;
	ThrowIfFailed(m_fanVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pFanVertexDataBegin)));
	memcpy(pFanVertexDataBegin, fanVertices.data(), fanVertexBufferSize);
	m_fanVertexBuffer->Unmap(0, nullptr);

	// 부채꼴 정점 버퍼 뷰 설정
	m_fanVertexBufferView.BufferLocation = m_fanVertexBuffer->GetGPUVirtualAddress();
	m_fanVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_fanVertexBufferView.SizeInBytes = fanVertexBufferSize;

	// 부채꼴 인덱스 버퍼 생성
	const UINT fanIndexBufferSize = static_cast<UINT>(fanIndices.size() * sizeof(UINT));
	bufferDesc.Width = fanIndexBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_fanIndexBuffer)
	));

	// 부채꼴 인덱스 데이터 복사
	UINT8* pFanIndexDataBegin;
	ThrowIfFailed(m_fanIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pFanIndexDataBegin)));
	memcpy(pFanIndexDataBegin, fanIndices.data(), fanIndexBufferSize);
	m_fanIndexBuffer->Unmap(0, nullptr);

	// 부채꼴 인덱스 버퍼 뷰 설정
	m_fanIndexBufferView.BufferLocation = m_fanIndexBuffer->GetGPUVirtualAddress();
	m_fanIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_fanIndexBufferView.SizeInBytes = fanIndexBufferSize;

	// 부채꼴 상수 버퍼 생성
	const UINT fanConstantBufferSize = (sizeof(ConstantBuffer) + 255) & ~255;
	bufferDesc.Width = fanConstantBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_fanConstantBuffer)
	));

	ThrowIfFailed(m_fanConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pFanCbvDataBegin)));
}

void LocalPlayer::RenderFan(
	ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection
)
{
	// 부채꼴 월드 행렬 계산
	XMMATRIX fanWorld = XMMatrixIdentity();

	Vec3 pos = GetPosition();
	Vec3 rot = GetRotation();
	fanWorld *= XMMatrixRotationY(rot.y);
	fanWorld *= XMMatrixTranslation(pos.x, pos.y, pos.z);
	
	// MVP 행렬 계산
	XMMATRIX fanMvp = fanWorld * view * projection;

	// 부채꼴 상수 버퍼 업데이트
	ConstantBuffer fanCbData;
	XMStoreFloat4x4(&fanCbData.mvp, XMMatrixTranspose(fanMvp));
	memcpy(m_pFanCbvDataBegin, &fanCbData, sizeof(fanCbData));

	// 부채꼴 상수 버퍼 바인딩
	cmdList->SetGraphicsRootConstantBufferView(0, m_fanConstantBuffer->GetGPUVirtualAddress());

	// 부채꼴 정점 및 인덱스 버퍼 설정
	cmdList->IASetVertexBuffers(0, 1, &m_fanVertexBufferView);
	cmdList->IASetIndexBuffer(&m_fanIndexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 부채꼴 그리기
	cmdList->DrawIndexedInstanced(m_fanIndexCount, 1, 0, 0, 0);
}

// 디버깅 원을 위한 초기화
void LocalPlayer::InitializeDebugCircle(ID3D12Device* device)
{
	// 원의 반지름 (NDC 좌표계: -1 ~ 1 범위)
	// 화면 크기에 상관없이 일정한 비율로 보이도록
	const float radius = 0.6f;		 // NDC 좌표계에서의 반지름
	const int	circleSegments = 64; // 원의 세밀도

	// 원의 버텍스 생성 (NDC 좌표계: 중심이 0,0)
	std::vector<Vertex> circleVertices;
	Vec4				circleColor(1.0f, 1.0f, 0.0f, 1.0f); // 노란색

	for (int i = 0; i <= circleSegments; i++)
	{
		float angle = 2.0f * XM_PI * i / circleSegments;
		float x = cosf(angle) * radius;
		float y = sinf(angle) * radius;
		// NDC 좌표계: Y축은 위가 양수
		circleVertices.push_back({{x, y, 0.0f}, circleColor});
	}

	// 원의 인덱스 (와이어프레임)
	std::vector<UINT> circleIndices;
	for (int i = 0; i < circleSegments; i++)
	{
		circleIndices.push_back(i);
		circleIndices.push_back(i + 1);
	}

	// 버텍스 버퍼 생성
	const UINT			  vertexBufferSize = circleVertices.size() * sizeof(Vertex);
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = vertexBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_debugCircleVertexBuffer)
	));

	UINT8*		pVertexDataBegin;
	D3D12_RANGE readRange = {0, 0};
	ThrowIfFailed(m_debugCircleVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, circleVertices.data(), vertexBufferSize);
	m_debugCircleVertexBuffer->Unmap(0, nullptr);

	m_debugCircleVertexBufferView.BufferLocation = m_debugCircleVertexBuffer->GetGPUVirtualAddress();
	m_debugCircleVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_debugCircleVertexBufferView.SizeInBytes = vertexBufferSize;

	// 인덱스 버퍼 생성
	const UINT indexBufferSize = circleIndices.size() * sizeof(UINT);
	bufferDesc.Width = indexBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_debugCircleIndexBuffer)
	));

	UINT8* pIndexDataBegin;
	ThrowIfFailed(m_debugCircleIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, circleIndices.data(), indexBufferSize);
	m_debugCircleIndexBuffer->Unmap(0, nullptr);

	m_debugCircleIndexBufferView.BufferLocation = m_debugCircleIndexBuffer->GetGPUVirtualAddress();
	m_debugCircleIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_debugCircleIndexBufferView.SizeInBytes = indexBufferSize;
	m_debugCircleIndexCount = circleIndices.size();

	// 상수 버퍼 생성
	const UINT constantBufferSize = sizeof(ConstantBuffer);
	bufferDesc.Width = constantBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_debugCircleConstantBuffer)
	));

	ThrowIfFailed(m_debugCircleConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pDebugCircleCbvDataBegin))
	);

	// 삼등분선 생성 (30도, 150도, 270도)
	std::vector<Vertex> lineVertices;
	Vec4				lineColor(0.0f, 1.0f, 1.0f, 1.0f); // 청록색

	// 중심점
	lineVertices.push_back({{0.0f, 0.0f, 0.0f}, lineColor});

	// 30도 선
	float angle30 = 30.0f * XM_PI / 180.0f;
	lineVertices.push_back({{cosf(angle30) * radius * 1.1f, sinf(angle30) * radius * 1.1f, 0.0f}, lineColor});

	// 150도 선
	float angle150 = 150.0f * XM_PI / 180.0f;
	lineVertices.push_back({{cosf(angle150) * radius * 1.1f, sinf(angle150) * radius * 1.1f, 0.0f}, lineColor});

	// 270도 선
	float angle270 = 270.0f * XM_PI / 180.0f;
	lineVertices.push_back({{cosf(angle270) * radius * 1.1f, sinf(angle270) * radius * 1.1f, 0.0f}, lineColor});

	// 선택 해제 영역 표시 (250도, 290도)
	Vec4  resetColor(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색
	float angle250 = 250.0f * XM_PI / 180.0f;
	lineVertices.push_back({{cosf(angle250) * radius * 1.1f, sinf(angle250) * radius * 1.1f, 0.0f}, resetColor});
	float angle290 = 290.0f * XM_PI / 180.0f;
	lineVertices.push_back({{cosf(angle290) * radius * 1.1f, sinf(angle290) * radius * 1.1f, 0.0f}, resetColor});

	// 선 인덱스 (중심에서 각도로)
	std::vector<UINT> lineIndices;
	lineIndices.push_back(0);
	lineIndices.push_back(1); // 30도
	lineIndices.push_back(0);
	lineIndices.push_back(2); // 150도
	lineIndices.push_back(0);
	lineIndices.push_back(3); // 270도
	lineIndices.push_back(0);
	lineIndices.push_back(4); // 250도
	lineIndices.push_back(0);
	lineIndices.push_back(5); // 290도

	// 선 버텍스 버퍼 생성
	const UINT lineVertexBufferSize = lineVertices.size() * sizeof(Vertex);
	bufferDesc.Width = lineVertexBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_debugLinesVertexBuffer)
	));

	ThrowIfFailed(m_debugLinesVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, lineVertices.data(), lineVertexBufferSize);
	m_debugLinesVertexBuffer->Unmap(0, nullptr);

	m_debugLinesVertexBufferView.BufferLocation = m_debugLinesVertexBuffer->GetGPUVirtualAddress();
	m_debugLinesVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_debugLinesVertexBufferView.SizeInBytes = lineVertexBufferSize;

	// 선 인덱스 버퍼 생성
	const UINT lineIndexBufferSize = lineIndices.size() * sizeof(UINT);
	bufferDesc.Width = lineIndexBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_debugLinesIndexBuffer)
	));

	ThrowIfFailed(m_debugLinesIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, lineIndices.data(), lineIndexBufferSize);
	m_debugLinesIndexBuffer->Unmap(0, nullptr);

	m_debugLinesIndexBufferView.BufferLocation = m_debugLinesIndexBuffer->GetGPUVirtualAddress();
	m_debugLinesIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_debugLinesIndexBufferView.SizeInBytes = lineIndexBufferSize;
	m_debugLinesIndexCount = lineIndices.size();

	// 선 상수 버퍼 생성
	bufferDesc.Width = constantBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_debugLinesConstantBuffer)
	));

	ThrowIfFailed(m_debugLinesConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pDebugLinesCbvDataBegin)));
}

void LocalPlayer::RenderDebugCircle(
	ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection
)
{
	// 전투 모드일 때만 렌더링
	if (m_cameraMode != CameraMode::COMBAT)
		return;

	// 화면 종횡비 계산
	float aspectRatio = 1.0f;
	if (m_hWnd != nullptr)
	{
		RECT clientRect;
		GetClientRect(m_hWnd, &clientRect);
		float width = static_cast<float>(clientRect.right - clientRect.left);
		float height = static_cast<float>(clientRect.bottom - clientRect.top);
		aspectRatio = width / height;
	}

	// 종횡비를 고려한 스케일 행렬 (X축을 종횡비로 나눠서 원을 원형으로 유지)
	DirectX::XMMATRIX aspectScale = DirectX::XMMatrixScaling(1.0f / aspectRatio, 1.0f, 1.0f);

	// 화면 좌표계로 렌더링
	DirectX::XMMATRIX screenView = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX screenProjection = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX screenWorld = DirectX::XMMatrixIdentity();

	// 종횡비 보정 적용
	DirectX::XMMATRIX screenMVP = screenWorld * aspectScale * screenView * screenProjection;

	// 원 렌더링
	cmdList->IASetVertexBuffers(0, 1, &m_debugCircleVertexBufferView);
	cmdList->IASetIndexBuffer(&m_debugCircleIndexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	DirectX::XMStoreFloat4x4(&m_debugCircleConstantBufferData.mvp, DirectX::XMMatrixTranspose(screenMVP));
	memcpy(m_pDebugCircleCbvDataBegin, &m_debugCircleConstantBufferData, sizeof(m_debugCircleConstantBufferData));

	cmdList->SetGraphicsRootConstantBufferView(0, m_debugCircleConstantBuffer->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(m_debugCircleIndexCount, 1, 0, 0, 0);

	// 삼등분선 렌더링
	cmdList->IASetVertexBuffers(0, 1, &m_debugLinesVertexBufferView);
	cmdList->IASetIndexBuffer(&m_debugLinesIndexBufferView);

	DirectX::XMStoreFloat4x4(&m_debugLinesConstantBufferData.mvp, DirectX::XMMatrixTranspose(screenMVP));
	memcpy(m_pDebugLinesCbvDataBegin, &m_debugLinesConstantBufferData, sizeof(m_debugLinesConstantBufferData));

	cmdList->SetGraphicsRootConstantBufferView(0, m_debugLinesConstantBuffer->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(m_debugLinesIndexCount, 1, 0, 0, 0);
}