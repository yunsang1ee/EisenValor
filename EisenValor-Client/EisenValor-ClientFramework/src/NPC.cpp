// NPC.cpp
#include "stdafxClientFramework.h"
#include "NPC.h"
#include "Vertex.h"
#include "GlobalInterfaces.h"

using namespace DirectX; 

void NPC::Initialize(ID3D12Device* device)
{
	DirectX::XMFLOAT4 color = GetTeamColor();

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
					  // 아래쪽
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
}

void NPC::Update(float deltaTime)
{
	const Vec3& curPos{GetPosition()};
	const Vec3& destPos{lastServerPosition};

	constexpr float epsilon{1e-6f};
	
	if ((fabs(curPos.x - destPos.x) <= epsilon) && (fabs(curPos.y - destPos.y) <= epsilon) &&
		(fabs(curPos.z - destPos.z) <= epsilon))
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
}

void NPC::SetTarget(std::shared_ptr<GameObject> target)
{
	m_target = target;
	if (target)
	{
		m_baseY = target->GetPosition().y; // 기준 높이 설정
	}
}

void NPC::UpdateUnitProperties()
{
	// 유닛 타입별 기본 속성 설정
	switch (m_unitType)
	{
	case NPC_TYPE::GENERAL:
		m_moveSpeed = 5.0f;
		break;
	case NPC_TYPE::SOLDIER:
		m_moveSpeed = 4.0f;
		break;
	case NPC_TYPE::BATTLE_RAM:
		m_moveSpeed = 3.0f;
		break;
	}
}

DirectX::XMFLOAT4 NPC::GetTeamColor() const
{
	if (m_team == Team::ALLY)
	{
		// 아군
		switch (m_unitType)
		{
		case NPC_TYPE::NONE:
			return DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f); // 한파랑
		case NPC_TYPE::SOLDIER:
			return DirectX::XMFLOAT4(0.3f	, 0.3f, 1.0f, 1.0f); // 연파랑
		case NPC_TYPE::BATTLE_RAM:
			return DirectX::XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f); // 하늘색
		}
	}
	else
	{
		// 적군
		switch (m_unitType)
		{
		case NPC_TYPE::GENERAL:
			return DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // 진빨강
		case NPC_TYPE::SOLDIER:
			return DirectX::XMFLOAT4(1.0f, 0.3f, 0.3f, 1.0f); // 연빨강
		case NPC_TYPE::BATTLE_RAM:
			return DirectX::XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f); // 주황
		}
	}
	return DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f); // 기본 회색
}

DirectX::XMFLOAT3 NPC::GetUnitScale() const
{
	switch (m_unitType)
	{
	case NPC_TYPE::GENERAL:
		return DirectX::XMFLOAT3(0.4f, 1.0f, 0.4f); // 장수
	case NPC_TYPE::SOLDIER:
		return DirectX::XMFLOAT3(0.2f, 0.5f, 0.2f); // 병사
	case NPC_TYPE::BATTLE_RAM:
		return DirectX::XMFLOAT3(0.8f, 0.5f, 1.2f); // 배틀램
	default:
		return DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
	}
}