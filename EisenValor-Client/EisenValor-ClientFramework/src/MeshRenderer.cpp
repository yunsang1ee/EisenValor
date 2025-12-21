#include "stdafxClientFramework.h"
#include "MeshRenderer.h"
#include "GameObject.h"

void MeshRenderer::Initialize(ID3D12Device* device)
{
	Vertex vertices[] = {
		{{-0.3f, -0.75f, -0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}}, // 좌하
		{{-0.3f, 0.75f, -0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}},  // 좌상
		{{0.3f, 0.75f, -0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}},   // 우상
		{{0.3f, -0.75f, -0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}},  // 우하

		{{-0.3f, -0.75f, 0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}}, // 좌하
		{{-0.3f, 0.75f, 0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}},  // 좌상
		{{0.3f, 0.75f, 0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}},	  // 우상
		{{0.3f, -0.75f, 0.3f}, {m_color.x, m_color.y, m_color.z, 1.0f}}	  // 우하
	};

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

void MeshRenderer::Render(
	ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection
)
{
	auto gameObj = GetGameObject();
	if (!gameObj)
		return;

	 DirectX::XMMATRIX worldMatrix = gameObj->GetTransform().GetWorldMatrix();


	// 메시 설정
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Transform에서 변환 행렬 계산
	const Vec3& pos = gameObj->GetPosition();
	const Vec3& rot = gameObj->GetRotation();

	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationY(rot.y);
	DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
	DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	DirectX::XMMATRIX mvp = worldMatrix * view * projection;

	// 상수 버퍼 업데이트
	DirectX::XMStoreFloat4x4(&m_constantBufferData.mvp, DirectX::XMMatrixTranspose(mvp));
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	// 메시 렌더링
	cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}