#include "stdafxClientFramework.h"
#include "LocalPlayer.h"
#include "InputGlobal.h"
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
}

void LocalPlayer::InitializeWireFrame(ID3D12Device* device)
{
	// 와이어프레임 박스 크기
	float halfW = 1.6f;
	float halfH = 1.0f;
	float halfD = 1.6f;

	// 8개 꼭짓점 (초록색 와이어프레임)
	DVertex vertices[] = {

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
	m_wireFrameVertexBufferView.StrideInBytes = sizeof(DVertex);
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

	DVertex vertices[] = {
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
	m_commandAreaVertexBufferView.StrideInBytes = sizeof(DVertex);
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
	targetPos.x = m_pos.x + forwardX * m_commandAreaDistance;
	targetPos.y = -0.5f; // 바닥 높이
	targetPos.z = m_pos.z + forwardZ * m_commandAreaDistance;

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

	// 와이어프레임 박스 렌더링
	// cmdList->IASetVertexBuffers(0, 1, &m_wireFrameVertexBufferView);
	// cmdList->IASetIndexBuffer(&m_wireFrameIndexBufferView);
	// cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); // 라인!

	//// 플레이어와 같은 위치, 회전
	// DirectX::XMMATRIX wireFrameRotation = DirectX::XMMatrixRotationY(m_rot.y);
	// DirectX::XMMATRIX wireFrameTranslation = DirectX::XMMatrixTranslation(m_pos.x, m_pos.y + 0.5f, m_pos.z);
	// DirectX::XMMATRIX wireFrameWorld = wireFrameRotation * wireFrameTranslation;
	// DirectX::XMMATRIX wireFrameMVP = wireFrameWorld * view * projection;

	// DirectX::XMStoreFloat4x4(&m_wireFrameConstantBufferData.mvp, DirectX::XMMatrixTranspose(wireFrameMVP));
	// memcpy(m_pWireFrameCbvDataBegin, &m_wireFrameConstantBufferData, sizeof(m_wireFrameConstantBufferData));

	// cmdList->SetGraphicsRootConstantBufferView(0, m_wireFrameConstantBuffer->GetGPUVirtualAddress());
	// cmdList->DrawIndexedInstanced(24, 1, 0, 0, 0); // 24개 인덱스 (12개 라인)
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
	int wheelDelta = MANAGER(InputGlobal).GetWheelScroll();
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


	if (MANAGER(InputGlobal).GetInputUp('W') || MANAGER(InputGlobal).GetInputUp('S') ||
		MANAGER(InputGlobal).GetInputUp('A') || MANAGER(InputGlobal).GetInputUp('D'))
	{
	}

	if (MANAGER(InputGlobal).GetInput('W')) // 전진
	{
		m_pos.x += forwardX * moveSpeed;
		m_pos.z += forwardZ * moveSpeed;
		// sendFlag = true;
	}
	else if (MANAGER(InputGlobal).GetInput('S')) // 후진
	{
		m_pos.x -= forwardX * moveSpeed;
		m_pos.z -= forwardZ * moveSpeed;
		// sendFlag = true;
	}
	else if (MANAGER(InputGlobal).GetInput('A')) // 좌측 이동
	{
		m_pos.x -= rightX * moveSpeed;
		m_pos.z -= rightZ * moveSpeed;
		//	sendFlag = true;
	}
	else if (MANAGER(InputGlobal).GetInput('D')) // 우측 이동
	{
		m_pos.x += rightX * moveSpeed;
		m_pos.z += rightZ * moveSpeed;
		//	sendFlag = true;
	}

	//// 수직 이동 (H/L 키)
	if (MANAGER(InputGlobal).GetInput('H'))
	{
		m_pos.y -= moveSpeed; // 아래로
							  //		sendFlag = true;
	}
	if (MANAGER(InputGlobal).GetInput('L'))
	{
		m_pos.y += moveSpeed; // 위로
							  //		sendFlag = true;
	}


	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	if (m_pos.x != lastX || m_pos.z != lastZ)
	{
		// DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n", m_pos.x, m_pos.y, m_pos.z);
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
		MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
		sendFlag = false;
	}
}

void LocalPlayer::UniformAcceleration(const float deltaTime)
{
	m_acceleration = Vec3{0, 0, 0}; // 매 프레임 초기화
	Vec3 forwardDir{sinf(m_cameraYaw), 0.f, cosf(m_cameraYaw)};
	Vec3 rightDir{sinf(m_cameraYaw + XM_PIDIV2), 0.f, cosf(m_cameraYaw + XM_PIDIV2)};

	constexpr float accelValue = 5.f;

	if (MANAGER(InputGlobal).GetInputDown('W'))
	{
		m_acceleration.x += forwardDir.x * accelValue;
		m_acceleration.y += forwardDir.y * accelValue;
		m_acceleration.z += forwardDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}

	else if (MANAGER(InputGlobal).GetInputDown('S'))
	{
		m_acceleration.x -= forwardDir.x * accelValue;
		m_acceleration.y -= forwardDir.y * accelValue;
		m_acceleration.z -= forwardDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}
	else if (MANAGER(InputGlobal).GetInputUp('W'))
	{
		sendFlag = false;
	}
	else if (MANAGER(InputGlobal).GetInputUp('S'))
	{
		sendFlag = false;
	}
	else if (MANAGER(InputGlobal).GetInputUp('A'))
	{
		sendFlag = false;
	}
	else if (MANAGER(InputGlobal).GetInputUp('D'))
	{
		sendFlag = false;
	}
	else if (MANAGER(InputGlobal).GetInputDown('A'))
	{
		m_acceleration.x -= rightDir.x * accelValue;
		m_acceleration.y -= rightDir.y * accelValue;
		m_acceleration.z -= rightDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}
	else if (MANAGER(InputGlobal).GetInputDown('D'))
	{
		m_acceleration.x += rightDir.x * accelValue;
		m_acceleration.y += rightDir.y * accelValue;
		m_acceleration.z += rightDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		sendFlag = true;
	}


	else if (MANAGER(InputGlobal).GetInput('W'))
	{
		m_acceleration.x += forwardDir.x * accelValue;
		m_acceleration.y += forwardDir.y * accelValue;
		m_acceleration.z += forwardDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;
		// sendFlag = true;
	}

	else if (MANAGER(InputGlobal).GetInput('S'))
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

	else if (MANAGER(InputGlobal).GetInput('A'))
	{
		m_acceleration.x -= rightDir.x * accelValue;
		m_acceleration.y -= rightDir.y * accelValue;
		m_acceleration.z -= rightDir.z * accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;

		// sendFlag = true;
	}

	else if (MANAGER(InputGlobal).GetInput('D'))
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
	else if (MANAGER(InputGlobal).GetInput('L'))
	{
		m_acceleration.y += accelValue;

		m_velocity.x += m_acceleration.x * deltaTime;
		m_velocity.y += m_acceleration.y * deltaTime;
		m_velocity.z += m_acceleration.z * deltaTime;

		//	sendFlag = true;
	}
	else if (MANAGER(InputGlobal).GetInput('H'))
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

	if (MANAGER(InputGlobal).GetInputDown('R'))
	{
		auto pb = NetBridge::ServerPacketHandler::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	}

	m_pos.x += m_velocity.x * deltaTime;
	m_pos.y += m_velocity.y * deltaTime;
	m_pos.z += m_velocity.z * deltaTime;

	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	if (m_pos.x != lastX || m_pos.z != lastZ)
	{
		// DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n", m_pos.x, m_pos.y, m_pos.z);
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

		// auto pb = NetBridge::ServerPacketHandler::Make_CS_MOVE_PACKET(pos, rot, vel, accel, timeStamp);
		// MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
		// sendFlag = false;
	}

	if (MANAGER(InputGlobal).GetInputDown('R'))
	{
		auto pb = NetBridge::ServerPacketHandler::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	}
}

void LocalPlayer::UpdateInput(const float deltaTime)
{
	const auto& input = MANAGER(InputGlobal);
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
			// DEBUG_LOG_FMT("Camera drag started at ({:.1f}, {:.1f})\n", mousePos.x, mousePos.y);
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
				// DEBUG_LOG_FMT(
				//	"Camera rotating - Delta({:.1f}, {:.1f}) Yaw: {:.2f}, Pitch: {:.2f}\n", deltaX, deltaY, m_cameraYaw,
				//	m_cameraPitch
				//);
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
			// DEBUG_LOG_FMT("Camera drag ended\n");
		}
	}
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
		auto pb = NetBridge::ServerPacketHandler::Make_CS_CHANGE_SOLDIER_FORMATION();
		MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
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
	if (MANAGER(InputGlobal).GetInputUp('W') || MANAGER(InputGlobal).GetInputUp('S') ||
		MANAGER(InputGlobal).GetInputUp('A') || MANAGER(InputGlobal).GetInputUp('D'))
	{
		m_velocity.x = 0.f;
		m_velocity.y = 0.f;
		m_velocity.z = 0.f;
		sendFlag = true;
	}


	if (MANAGER(InputGlobal).GetInputDown('R'))
	{
		auto pb = NetBridge::ServerPacketHandler::Make_CS_SUMMON_NPC_PACKET();
		MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	}

	if (MANAGER(InputGlobal).GetInputDown('Q'))
	{
		auto pb = NetBridge::ServerPacketHandler::Make_CS_PLAYER_ATTACK_PACKET();
		MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	}

	if (MANAGER(InputGlobal).GetInputDown('T'))
	{
		// auto pos = GetPosition();

		//// 플레이어가 바라보는 방향 벡터 (XZ 평면)
		// const float forwardX = sinf(m_cameraYaw);
		// const float forwardZ = cosf(m_cameraYaw);

		//// 5.f 만큼 떨어진 위치
		// pos.x += forwardX * 5.f;
		// pos.z += forwardZ * 5.f;
		Vec3 pos;
		auto pb = NetBridge::ServerPacketHandler::Make_CS_SOLDIER_MOVE_PACKET(pos);
		MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	}

	// if(input.GetInputDown(VK_F1)) {
	//	auto pb = NetBridge::ServerPacketHandler::Make_CS_SOLDIER_FORMATION(SOLDIER_FORMATION::FORMATION_1);
	//	MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	// }
	// else if(input.GetInputDown(VK_F2)) {
	//	auto pb = NetBridge::ServerPacketHandler::Make_CS_SOLDIER_FORMATION(SOLDIER_FORMATION::FORMATION_2);
	//	MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	// }
	// else if (input.GetInputDown(VK_F3))
	//{
	//	auto pb = NetBridge::ServerPacketHandler::Make_CS_SOLDIER_FORMATION(SOLDIER_FORMATION::FORMATION_3);
	//	MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
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

void LocalPlayer::UpdatePos(const float deltaTime)
{
	m_pos.x += m_velocity.x * deltaTime;
	m_pos.y += m_velocity.y * deltaTime;
	m_pos.z += m_velocity.z * deltaTime;
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

	auto pb = NetBridge::ServerPacketHandler::Make_CS_MOVE_PACKET(true, true, pos, rot, vel, accel, timeStamp);
	MANAGER(NetBridge::NetworkManager).Send(std::move(pb));
	sendFlag = false;
}

void LocalPlayer::InitializeFan(ID3D12Device* device)
{
	// 부채꼴 정점 데이터 생성
	std::vector<DVertex> fanVertices;
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
	const UINT			fanVertexBufferSize = static_cast<UINT>(fanVertices.size() * sizeof(DVertex));
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
	m_fanVertexBufferView.StrideInBytes = sizeof(DVertex);
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

	// 플레이어의 Y축 회전(Yaw) 적용 - 플레이어가 바라보는 방향으로 부채꼴 향하게 함
	fanWorld *= XMMatrixRotationY(m_rot.y);

	// 플레이어 위치로 이동
	fanWorld *= XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

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