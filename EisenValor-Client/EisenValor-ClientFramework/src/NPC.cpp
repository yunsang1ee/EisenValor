// NPC.cpp
#include "stdafxClientFramework.h"
#include "NPC.h"
#include "Vertex.h"
#include "GlobalInterfaces.h"

using namespace DirectX;

void NPC::Initialize(ID3D12Device* device)
{
	DirectX::XMFLOAT4 color = m_teamColor;

	Vertex vertices[] = {// 전면
						 {DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), color},
						 {DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f), color},
						 {DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f), color},
						 {DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f), color},
						 // 후면
						 {DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), color},
						 {DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), color},
						 {DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), color},
						 {DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), color}
	};

	// 인덱스 데이터
	UINT indices[] = {// 앞면
					  0, 1, 2, 0, 2, 3,
					  // 뒷면
					  4, 6, 5, 4, 7, 6,
					  // 왼쪽면
					  0, 5, 1, 0, 4, 5,
					  // 오른쪽면
					  3, 2, 6, 3, 6, 7,
					  // 위쪽
					  1, 5, 6, 1, 6, 2,
					  // 아래쪽r
					  0, 3, 7, 0, 7, 4
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

	// 상수 버퍼 생성
	const UINT constantBufferSize = (sizeof(ConstantBuffer) + 255) & ~255;
	bufferDesc.Width = constantBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_constantBuffer)
	));

	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));

	// HP바 수치용 버텍스 데이터
	Vec4   hpBarForegroundColor(0.54f, 0.03f, 0.03f, 1.0f); // 빨간색
	Vertex hpBarForegroundVertices[] = {					
										// 전면
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

	// HP바 수치 상수 버퍼 생성 (MVP 행렬용)
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
}

void NPC::Update(float deltaTime)
{
	const Vec3& curPos{GetPosition()};
	const Vec3& destPos{lastServerPosition};

	constexpr float epsilon{1e-6f};
	
	if ((fabs(curPos.x - destPos.x) <= epsilon) && (fabs(curPos.y - destPos.y) <= epsilon) && (fabs(curPos.z - destPos.z) <= epsilon))
		return;

	float lerpFactor = std::min(deltaTime * 5.f, 1.f); // 현재 위치와 목적지 사이 거리의 비율

	Vec3 newPos{SmoothLerp(curPos, destPos, lerpFactor)};

	SetPosition(newPos);
}

void NPC::Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// 버텍스 및 인덱스 버퍼 설정
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DirectX::XMFLOAT3 scale = GetUnitScale();

	// NPC 변환 행렬 계산
	DirectX::XMMATRIX npcScale = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX npcRotation = DirectX::XMMatrixRotationY(m_rot.y);
	DirectX::XMMATRIX npcTranslation = DirectX::XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	DirectX::XMMATRIX npcWorld = npcScale * npcRotation * npcTranslation;
	DirectX::XMMATRIX npcMVP = npcWorld * view * projection;

	// 상수 버퍼에 데이터 복사
	DirectX::XMStoreFloat4x4(&m_constantBufferData.mvp, DirectX::XMMatrixTranspose(npcMVP));
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	// NPC 그리기
	cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	// HP바 렌더링 추가
	float hpRatio = GetHPRatio();
	if (hpRatio > 0.0f)
	{
		// 빌보드 행렬 계산
		DirectX::XMMATRIX billboardMatrix = DirectX::XMMatrixIdentity();

		billboardMatrix.r[0] = DirectX::XMVectorSet(
			DirectX::XMVectorGetX(view.r[0]), DirectX::XMVectorGetX(view.r[1]), DirectX::XMVectorGetX(view.r[2]), 0.0f
		);
		billboardMatrix.r[1] = DirectX::XMVectorSet(
			DirectX::XMVectorGetY(view.r[0]), DirectX::XMVectorGetY(view.r[1]), DirectX::XMVectorGetY(view.r[2]), 0.0f
		);
		billboardMatrix.r[2] = DirectX::XMVectorSet(
			DirectX::XMVectorGetZ(view.r[0]), DirectX::XMVectorGetZ(view.r[1]), DirectX::XMVectorGetZ(view.r[2]), 0.0f
		);

		// HP바 전용 버텍스 버퍼로 변경
		cmdList->IASetVertexBuffers(0, 1, &m_hpBarVertexBufferView);

		float hpBarWidth = 0.8f * hpRatio; // NPC는 플레이어보다 작게

		// NPC 크기에 맞춰 HP바 위치 조정
		DirectX::XMFLOAT3 scale = GetUnitScale();
		float			  yOffset = scale.y + 0.3f; // NPC 높이 + 여유공간

		DirectX::XMMATRIX hpBarForegroundOffset = DirectX::XMMatrixTranslation(0.0f, yOffset, 0.01f);
		DirectX::XMMATRIX hpBarForegroundScale = DirectX::XMMatrixScaling(hpBarWidth, 0.2f, 0.02f);
		DirectX::XMMATRIX hpBarForegroundWorld =
			hpBarForegroundScale * billboardMatrix * hpBarForegroundOffset * npcTranslation;
		DirectX::XMMATRIX hpBarForegroundMVP = hpBarForegroundWorld * view * projection;

		// HP바 수치 상수 버퍼에 업데이트
		DirectX::XMStoreFloat4x4(&m_hpBarForegroundData.mvp, DirectX::XMMatrixTranspose(hpBarForegroundMVP));
		memcpy(m_pHpBarForegroundDataBegin, &m_hpBarForegroundData, sizeof(m_hpBarForegroundData));

		// HP바 수치 그리기
		cmdList->SetGraphicsRootConstantBufferView(0, m_hpBarForegroundBuffer->GetGPUVirtualAddress());
		cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

		// 원래 NPC 버텍스 버퍼로 복원
		cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	}

}

void NPC::SetTarget(std::shared_ptr<GameObject> target)
{
	m_target = target;
	if (target)
	{
		m_baseY = target->GetPosition().y; // 기준 높이 설정
	}
}

void NPC::SetTeamColor() 
{
	switch (m_team)
	{
	case FB_ENUMS::TEAM_TYPE_BLUE:
		switch (m_unitType)
		{
		case FB_ENUMS::NPC_TYPE_GENERAL:
		{
			m_teamColor = Vec4(0.3f, 0.3f, 1.0f, 1.0f);
		}
			break;
		case FB_ENUMS::NPC_TYPE_SOLDIER:
		{
			m_teamColor = Vec4(0.5f, 0.5f, 1.0f, 1.0f);
			break;
		}
		default:
			break;
		}
		break;
	case FB_ENUMS::TEAM_TYPE_RED:
		switch (m_unitType)
		{
		case FB_ENUMS::NPC_TYPE_GENERAL:
		{
			m_teamColor = Vec4(1.0f, 0.3f, 0.3f, 1.0f);
		}
		break;
		case FB_ENUMS::NPC_TYPE_SOLDIER:
		{
			m_teamColor = Vec4(1.0f, 0.5f, 0.5f, 1.0f);
			break;
		}
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void NPC::UpdateUnitProperties()
{
	// 유닛 타입별 기본 속성 설정
	switch (m_unitType)
	{
	case FB_ENUMS::NPC_TYPE_GENERAL:
		m_moveSpeed = 5.0f;
		break;
	case FB_ENUMS::NPC_TYPE_SOLDIER:
		m_moveSpeed = 4.0f;
		break;
		break;
	}
}

DirectX::XMFLOAT3 NPC::GetUnitScale() const
{
	switch (m_unitType)
	{
	case FB_ENUMS::NPC_TYPE_GENERAL:
		return DirectX::XMFLOAT3(0.4f, 1.0f, 0.4f); // 장수
	case FB_ENUMS::NPC_TYPE_SOLDIER:
		return DirectX::XMFLOAT3(0.2f, 0.5f, 0.2f); // 병사
	default:
		return DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
	}
}