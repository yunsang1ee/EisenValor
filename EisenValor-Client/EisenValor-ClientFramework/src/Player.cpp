#include "stdafxClientFramework.h"
#include "Player.h"
#include "Vertex.h"
#include "GlobalInterfaces.h"

using namespace DirectX;

void Player::Initialize(ID3D12Device* device)
{						 // 큐브 버텍스 데이터 (GameFramework에서 사용하던 것과 동일)
	Vertex vertices[] = {// 전면
						 {DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
						 {DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
						 {DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
						 {DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
						 // 후면
						 {DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)},
						 {DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)},
						 {DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)},
						 {DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)}
	};
	m_vertices.assign_range(vertices);

	// 인덱스 데이터
	UINT indices[] = {// 전면
					  0, 1, 2, 0, 2, 3,
					  // 후면
					  4, 6, 5, 4, 7, 6,
					  // 좌측면
					  0, 5, 1, 0, 4, 5,
					  // 우측면
					  3, 2, 6, 3, 6, 7,
					  // 상단
					  1, 5, 6, 1, 6, 2,
					  // 하단
					  0, 3, 7, 0, 7, 4
	};
	m_indices.assign_range(indices);

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
		IID_PPV_ARGS(&m_vertexBuffer)
	));

	// 버텍스 데이터 복사
	UINT8*		pVertexDataBegin;
	D3D12_RANGE readRange = {0, 0};
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, vertices, sizeof(vertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// 버텍스 버퍼 뷰 설정
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	// 인덱스 버퍼 생성
	const UINT indexBufferSize = sizeof(indices);
	bufferDesc.Width = indexBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_indexBuffer)
	));

	// 인덱스 데이터 복사
	UINT8* pIndexDataBegin;
	ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices, sizeof(indices));
	m_indexBuffer->Unmap(0, nullptr);

	// 인덱스 버퍼 뷰 설정
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = indexBufferSize;

	// 플레이어 상수 버퍼 생성
	const UINT constantBufferSize = (sizeof(ConstantBuffer) + 255) & ~255;
	bufferDesc.Width = constantBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_constantBuffer)
	));

	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));

	// 표시등 상수 버퍼 생성
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_constantBuffer3)
	));

	ThrowIfFailed(m_constantBuffer3->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin3)));
}

void Player::Update(float deltaTime)
{
#ifdef DEAD_RECKONING
	if (keyup)
	{
		const float dampingFactor = 0.95f;

		// 직접 곱해줌 (operator *= 없이)
		m_velocity.x = m_velocity.x * dampingFactor;
		m_velocity.y = m_velocity.y * dampingFactor;
		m_velocity.z = m_velocity.z * dampingFactor;

		// 속도 크기 제곱 직접 계산 (LengthSquared)
		float speedSq = m_velocity.x * m_velocity.x + m_velocity.y * m_velocity.y + m_velocity.z * m_velocity.z;

		// 아주 느려지면 스냅 (멈춘 걸로 처리)
		if (speedSq < 0.001f)
		{
			m_velocity.x = 0.f;
			m_velocity.y = 0.f;
			m_velocity.z = 0.f;
		}

		// 위치 이동 적용
		m_pos.x = m_pos.x + m_velocity.x * deltaTime;
		m_pos.y = m_pos.y + m_velocity.y * deltaTime;
		m_pos.z = m_pos.z + m_velocity.z * deltaTime;
	}
	else
	{
		const uint64 now = std::chrono::duration_cast<std::chrono::milliseconds>(
							   std::chrono::high_resolution_clock::now().time_since_epoch()
		)
							   .count();
		const double serverDT = (now - lastServerTimestamp) / 1000.f;

		const Vec3 predictedPosition =
			PredictPosition(lastServerPosition, lastServerVelocity, lastServerAcceleration, serverDT);

		if (m_pos.x == predictedPosition.x && m_pos.y == predictedPosition.y && m_pos.z == predictedPosition.z)
			return;

		// 보간 계수 계산 (deltaTime 기반)
		const float smoothingSpeed = 10.0f; // 높을수록 빠르게 수렴
		const float alpha = 1.0f - std::exp(-smoothingSpeed * deltaTime);

		m_pos = Lerp(m_pos, predictedPosition, alpha);
	}
#endif
	Vec3 curPos{GetPosition()};
	Vec3 destPos{lastServerPosition};

	if (curPos.x == destPos.x && curPos.y == destPos.y && curPos.z == destPos.z)
		return;

	float lerpFactor = deltaTime * 5.f; // speed: 초당 이동 비율 (0~1 이상 가능)
	if (lerpFactor > 1.0f)
		lerpFactor = 1.0f; // 목적지 overshoot 방지

	Vec3 newPos;
	newPos.x = curPos.x + (destPos.x - curPos.x) * lerpFactor;
	newPos.y = curPos.y + (destPos.y - curPos.y) * lerpFactor;
	newPos.z = curPos.z + (destPos.z - curPos.z) * lerpFactor;
	SetPosition(newPos);

	//{
	//	Vec3 curEuler = GetRotation();		 // 현재 회전 (deg)
	//	Vec3 destEuler = lastServerRotation; // 목표 회전 (deg)

	//	float t = deltaTime * 8.f;
	//	if (t > 1.f)
	//		t = 1.f;

	//	// 1. 오일러(deg) → 쿼터니언
	//	XMVECTOR curQuat = XMQuaternionRotationRollPitchYaw(
	//		XMConvertToRadians(curEuler.x), XMConvertToRadians(curEuler.y), XMConvertToRadians(curEuler.z)
	//	);
	//	XMVECTOR destQuat = XMQuaternionRotationRollPitchYaw(
	//		XMConvertToRadians(destEuler.x), XMConvertToRadians(destEuler.y), XMConvertToRadians(destEuler.z)
	//	);

	//	// 2. 쿼터니언 보간
	//	XMVECTOR newQuat = XMQuaternionSlerp(curQuat, destQuat, t);

	//	// 3. 쿼터니언 → 행렬
	//	XMMATRIX rotMat = XMMatrixRotationQuaternion(newQuat);

	//	// 4. 행렬 → 오일러(deg)
	//	Vec3 newEuler;
	//	newEuler.y = XMConvertToDegrees(atan2f(rotMat.r[0].m128_f32[2], rotMat.r[2].m128_f32[2])); // yaw
	//	newEuler.x = XMConvertToDegrees(asinf(-rotMat.r[1].m128_f32[2]));						   // pitch
	//	newEuler.z = XMConvertToDegrees(atan2f(rotMat.r[1].m128_f32[0], rotMat.r[1].m128_f32[1])); // roll
	//	SetRotation(newEuler);
	//}
}

void Player::Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// 버텍스 및 인덱스 버퍼 설정
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 플레이어 큐브 렌더링
	DirectX::XMMATRIX playerScale = DirectX::XMMatrixScaling(0.3f, 0.8f, 0.3f);
	DirectX::XMMATRIX playerRotation = DirectX::XMMatrixRotationY(m_rot.y);
	DirectX::XMMATRIX playerTranslation = DirectX::XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	DirectX::XMMATRIX playerWorld = playerScale * playerRotation * playerTranslation;
	DirectX::XMMATRIX playerMVP = playerWorld * view * projection;

	// 플레이어 상수 버퍼에 복사
	DirectX::XMStoreFloat4x4(&m_constantBufferData.mvp, DirectX::XMMatrixTranspose(playerMVP));
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	// 플레이어 큐브 그리기
	cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	// 표시등 큐브 렌더링
	DirectX::XMMATRIX markerOffset = DirectX::XMMatrixTranslation(0.0f, 0.2f, 0.2f);
	DirectX::XMMATRIX markerScale = DirectX::XMMatrixScaling(0.1f, 0.1f, 0.1f);
	DirectX::XMMATRIX markerWorld = markerScale * markerOffset * playerRotation * playerTranslation;
	DirectX::XMMATRIX markerMVP = markerWorld * view * projection;

	// 표시등 상수 버퍼에 업데이트
	DirectX::XMStoreFloat4x4(&m_constantBufferData3.mvp, DirectX::XMMatrixTranspose(markerMVP));
	memcpy(m_pCbvDataBegin3, &m_constantBufferData3, sizeof(m_constantBufferData3));

	// 표시등 큐브 그리기
	cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer3->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

DirectX::XMMATRIX Player::GetViewMatrix() const
{
	float camX = m_pos.x - m_cameraDistance * sinf(m_cameraYaw) * cosf(m_cameraPitch);
	float camY = m_pos.y + 3.0f + m_cameraDistance * sinf(m_cameraPitch);
	float camZ = m_pos.z - m_cameraDistance * cosf(m_cameraYaw) * cosf(m_cameraPitch);

	float lookX = m_pos.x + 2.0f * sinf(m_cameraYaw);
	float lookY = m_pos.y + 1.0f + 2.0f * sinf(m_cameraPitch);
	float lookZ = m_pos.z + 2.0f * cosf(m_cameraYaw);

	return DirectX::XMMatrixLookAtLH(
		DirectX::XMVectorSet(camX, camY, camZ, 0.0f), DirectX::XMVectorSet(lookX, lookY, lookZ, 0.0f),
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	);
}