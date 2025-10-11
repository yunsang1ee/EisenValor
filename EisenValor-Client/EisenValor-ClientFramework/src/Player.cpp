#include "stdafxClientFramework.h"
#include "Player.h"
#include "Vertex.h"
#include "GlobalInterfaces.h"

using namespace DirectX;

void Player::Initialize(ID3D12Device* device)
{						 // 큐브 버텍스 데이터 
	Vertex vertices[] = {// 전면
						 {DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f),m_teamColor},
						 {DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f), m_teamColor},
						 {DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f), m_teamColor},
						 {DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f), m_teamColor},
						 // 후면
						 {DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), m_teamColor},
						 {DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), m_teamColor},
						 {DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), m_teamColor},
						 {DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), m_teamColor}
	};

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


    // HP바 배경용 버텍스 데이터
	Vec4   hpBarBackgroundColor(0.0f, 0.0f, 1.0f, 1.0f);
	Vertex hpBarBackgroundVertices[] = {				 // 전면
										{DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), hpBarBackgroundColor},
										{DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f), hpBarBackgroundColor},
										{DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f), hpBarBackgroundColor},
										{DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f), hpBarBackgroundColor},
										// 후면
										{DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), hpBarBackgroundColor},
										{DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), hpBarBackgroundColor},
										{DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), hpBarBackgroundColor},
										{DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), hpBarBackgroundColor}
	};

	// HP바 수치용 버텍스 데이터
	Vec4   hpBarForegroundColor(0.54f, 0.03f, 0.03f, 1.0f); //
	Vertex hpBarForegroundVertices[] = {				 // 전면
										{DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), hpBarForegroundColor},
										{DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f), hpBarForegroundColor},
										{DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f), hpBarForegroundColor},
										{DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f), hpBarForegroundColor},
										// 후면
										{DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), hpBarForegroundColor},
										{DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), hpBarForegroundColor},
										{DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), hpBarForegroundColor},
										{DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), hpBarForegroundColor}
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

	// // HP바 배경 상수 버퍼 생성
	//ThrowIfFailed(device->CreateCommittedResource(
	//	&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
	//	IID_PPV_ARGS(&m_hpBarBackgroundBuffer)
	//));
	//ThrowIfFailed(m_hpBarBackgroundBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pHpBarBackgroundDataBegin)));
	// HP바 전경 상수 버퍼 생성 (MVP 행렬용)
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_hpBarForegroundBuffer)
	));
	ThrowIfFailed(m_hpBarForegroundBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pHpBarForegroundDataBegin)));

	// HP바 전용 버텍스 버퍼 생성
	const UINT hpBarVertexBufferSize = sizeof(hpBarForegroundVertices);
	bufferDesc.Width = hpBarVertexBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_hpBarVertexBuffer)
	));

	// HP바 버텍스 데이터 복사
	UINT8* pHpBarVertexDataBegin;
	ThrowIfFailed(m_hpBarVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pHpBarVertexDataBegin)));
	memcpy(pHpBarVertexDataBegin, hpBarForegroundVertices, sizeof(hpBarForegroundVertices));
	m_hpBarVertexBuffer->Unmap(0, nullptr);

	// HP바 버텍스 버퍼 뷰 설정
	m_hpBarVertexBufferView.BufferLocation = m_hpBarVertexBuffer->GetGPUVirtualAddress();
	m_hpBarVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_hpBarVertexBufferView.SizeInBytes = hpBarVertexBufferSize;

	// HP바 전경 버텍스 데이터 복사
	ThrowIfFailed(m_hpBarForegroundBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, hpBarForegroundVertices, sizeof(hpBarForegroundVertices));
	m_hpBarForegroundBuffer->Unmap(0, nullptr);

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
	#else

	
	{
		if (m_rot.y == lastServerRotation.y)
			return;
		const Vec3	now = m_rot;				 // 현재 오일러 회전 (deg)
		const Vec3	future = lastServerRotation; // 목표 오일러 회전 (deg)
		const float t = 1.0f - std::exp(-10.f * deltaTime);

		Vec3 result;
		result.x = now.x + (future.x - now.x) * t;
		result.y = now.y + (future.y - now.y) * t;
		result.z = now.z + (future.z - now.z) * t;

		SetRotation(result);
	}

	Vec3 curPos{GetPosition()};
	Vec3 destPos{lastServerPosition};

	if ((curPos.x == destPos.x && curPos.y == destPos.y && curPos.z == destPos.z))
		return;

	float lerpFactor = deltaTime * 5.f; // speed: 초당 이동 비율 (0~1 이상 가능)
	if (lerpFactor > 1.0f)
		lerpFactor = 1.0f; // 목적지 overshoot 방지

	Vec3 newPos;
	newPos.x = curPos.x + (destPos.x - curPos.x) * lerpFactor;
	newPos.y = curPos.y + (destPos.y - curPos.y) * lerpFactor;
	newPos.z = curPos.z + (destPos.z - curPos.z) * lerpFactor;
	SetPosition(newPos);

#endif
}

void Player::Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// 버텍스 및 인덱스 버퍼 설정
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 플레이어 큐브 렌더링
	DirectX::XMMATRIX playerScale = DirectX::XMMatrixScaling(0.5f, 1.2f, 0.5f);
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
	DirectX::XMMATRIX markerOffset = DirectX::XMMatrixTranslation(0.0f, 0.4f, 0.3f);
	DirectX::XMMATRIX markerScale = DirectX::XMMatrixScaling(0.2f, 0.2f, 0.2f);
	DirectX::XMMATRIX markerWorld = markerScale * markerOffset * playerRotation * playerTranslation;
	DirectX::XMMATRIX markerMVP = markerWorld * view * projection;

	// 표시등 상수 버퍼에 업데이트
	DirectX::XMStoreFloat4x4(&m_constantBufferData3.mvp, DirectX::XMMatrixTranspose(markerMVP));
	memcpy(m_pCbvDataBegin3, &m_constantBufferData3, sizeof(m_constantBufferData3));

	// 표시등 큐브 그리기
	cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer3->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	// HP바 배경 렌더링
	//DirectX::XMMATRIX hpBarBackgroundOffset = DirectX::XMMatrixTranslation(0.0f, 1.8f, 0.0f);
	//DirectX::XMMATRIX hpBarBackgroundScale = DirectX::XMMatrixScaling(1.0f, 0.1f, 0.05f);
	// 
	//// 빌보드 행렬: 뷰 행렬의 회전 부분을 역으로 적용
	DirectX::XMMATRIX billboardMatrix = DirectX::XMMatrixIdentity();
	
	// 뷰 행렬의 회전 부분만 추출해서 전치
	// XMMATRIX는 r[0], r[1], r[2], r[3] 형태
	billboardMatrix.r[0] = DirectX::XMVectorSet(
		DirectX::XMVectorGetX(view.r[0]), DirectX::XMVectorGetX(view.r[1]), DirectX::XMVectorGetX(view.r[2]), 0.0f
	);
	billboardMatrix.r[1] = DirectX::XMVectorSet(
		DirectX::XMVectorGetY(view.r[0]), DirectX::XMVectorGetY(view.r[1]), DirectX::XMVectorGetY(view.r[2]), 0.0f
	);
	billboardMatrix.r[2] = DirectX::XMVectorSet(
		DirectX::XMVectorGetZ(view.r[0]), DirectX::XMVectorGetZ(view.r[1]), DirectX::XMVectorGetZ(view.r[2]), 0.0f
	);
	//// 빌보드 행렬을 적용
	//DirectX::XMMATRIX hpBarBackgroundWorld =
	//	hpBarBackgroundScale * billboardMatrix * hpBarBackgroundOffset * playerTranslation;
	//DirectX::XMMATRIX hpBarBackgroundMVP = hpBarBackgroundWorld * view * projection;

	//// HP바 배경 상수 버퍼에 업데이트
	//DirectX::XMStoreFloat4x4(&m_hpBarBackgroundData.mvp, DirectX::XMMatrixTranspose(hpBarBackgroundMVP));
	//memcpy(m_pHpBarBackgroundDataBegin, &m_hpBarBackgroundData, sizeof(m_hpBarBackgroundData));

	//// HP바 배경 그리기
	//cmdList->SetGraphicsRootConstantBufferView(0, m_hpBarBackgroundBuffer->GetGPUVirtualAddress());
	//cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	// HP 수치 렌더링
	float hpRatio = GetHPRatio();
	if (hpRatio > 0.0f)
	{
		// HP바 전용 버텍스 버퍼로 변경
		cmdList->IASetVertexBuffers(0, 1, &m_hpBarVertexBufferView);
		
		float hpBarWidth = 2.0f * hpRatio;
		float hpBarOffsetX = -0.5f * (1.0f - hpRatio);
		
		DirectX::XMMATRIX hpBarForegroundOffset = DirectX::XMMatrixTranslation(hpBarOffsetX, 1.8f, 0.01f);
		DirectX::XMMATRIX hpBarForegroundScale = DirectX::XMMatrixScaling(hpBarWidth, 0.5f, 0.05f);
		DirectX::XMMATRIX hpBarForegroundWorld =
			hpBarForegroundScale * billboardMatrix * hpBarForegroundOffset * playerTranslation;
		DirectX::XMMATRIX hpBarForegroundMVP = hpBarForegroundWorld * view * projection;

		// HP바 수치 상수 버퍼에 업데이트
		DirectX::XMStoreFloat4x4(&m_hpBarForegroundData.mvp, DirectX::XMMatrixTranspose(hpBarForegroundMVP));
		memcpy(m_pHpBarForegroundDataBegin, &m_hpBarForegroundData, sizeof(m_hpBarForegroundData));

		// HP바 수치 그리기
		cmdList->SetGraphicsRootConstantBufferView(0, m_hpBarForegroundBuffer->GetGPUVirtualAddress());
		cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
		
		// 원래 플레이어 버텍스 버퍼로 복원
		cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	}

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

void Player::SetTeamColor() 
{
	switch (m_team)
	{
	case FB_ENUMS::TEAM_TYPE_BLUE:
	{
		m_teamColor = Vec4(0.0f, 0.0f, 1.0f, 1.0f); // 파랑
		break;
	}
	case FB_ENUMS::TEAM_TYPE_RED:
	{
		m_teamColor = Vec4(1.0f, 0.0f, 0.0f, 1.0f); // 빨강
		break;
	}
	default:
		break;
	}
}

