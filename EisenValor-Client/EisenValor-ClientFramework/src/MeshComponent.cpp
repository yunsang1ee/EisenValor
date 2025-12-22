#include "stdafxClientFramework.h"
#include "MeshComponent.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "Transform.h"

// 헬퍼 함수: GameObject 포인터 얻기
GameObject* MeshComponent::GetGameObject() const
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	if (!scene)
		return nullptr;

	auto* myGameObject = scene->TryGetGameObject(GetOwner());
	return myGameObject;
}

void MeshComponent::SetMesh(
	const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, std::string_view name
)
{
	m_vertices = vertices;
	m_indices = indices;

	auto* myGameObject = GetGameObject();
	if (myGameObject)
	{
		this->m_name = name.empty() ? myGameObject->GetName() + "_Mesh" : std::string(name);
	}
	else
	{
		this->m_name = name.empty() ? "Mesh" : std::string(name);
	}
}

void MeshComponent::BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, DxUploadHeap* uploadHeap)
{
	assert(!m_vertices.empty() && "[MeshComponent] No vertices set");

	const uint64_t vbSize = m_vertices.size() * sizeof(Vertex);
	auto		   vbUpload = uploadHeap->UploadRawData(m_vertices.data(), vbSize, 16);

	D3D12_RESOURCE_DESC vbDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = vbSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc{.Count = 1, .Quality = 0},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	D3D12_HEAP_PROPERTIES defaultHeap = {};
	defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer)
	));
	auto barrierToCopy = DxUtils::CreateTransitionBarrier(
		m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
	);
	cmdList->ResourceBarrier(1, &barrierToCopy);

	auto debugName = "VertexBuffer_" + m_name;
	m_vertexBuffer->SetName(std::wstring(debugName.begin(), debugName.end()).c_str());
	cmdList->CopyBufferRegion(m_vertexBuffer.Get(), 0, uploadHeap->GetResource(), vbUpload.offset, vbSize);

	auto barrier = DxUtils::CreateTransitionBarrier(
		m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
	);
	cmdList->ResourceBarrier(1, &barrier);

	m_vertexBufferGPU = m_vertexBuffer->GetGPUVirtualAddress();

	if (!m_indices.empty())
	{
		const uint64_t ibSize = m_indices.size() * sizeof(uint32_t);
		auto		   ibUpload = uploadHeap->UploadRawData(m_indices.data(), ibSize, 4);

		D3D12_RESOURCE_DESC ibDesc = vbDesc;
		ibDesc.Width = ibSize;

		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeap, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_PPV_ARGS(&m_indexBuffer)
		));
		auto barrierToCopy = DxUtils::CreateTransitionBarrier(
			m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrierToCopy);

		debugName = "IndexBuffer_" + m_name;
		m_indexBuffer->SetName(std::wstring(debugName.begin(), debugName.end()).c_str());

		cmdList->CopyBufferRegion(m_indexBuffer.Get(), 0, uploadHeap->GetResource(), ibUpload.offset, ibSize);

		barrier = DxUtils::CreateTransitionBarrier(
			m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier);

		m_indexBufferGPU = m_indexBuffer->GetGPUVirtualAddress();
	}

	m_blas = std::make_unique<DxBLAS>();
	m_blas->Build(
		device, cmdList, m_vertexBufferGPU, GetVertexCount(), sizeof(Vertex), m_indexBufferGPU, GetIndexCount(), false,
		m_name
	);

	DEBUG_LOG_FMT("[MeshComponent] Built BLAS: {} vertices, {} indices\n", GetVertexCount(), GetIndexCount());
}

void MeshComponent::Initialize(ID3D12Device* device)
{
	if (m_vertices.empty())
	{
		DEBUG_LOG_FMT("[MeshComponent] Initialize called but no vertices set!\n");
		return;
	}

	// 버텍스 버퍼 생성 (UPLOAD heap)
	const UINT			  vertexBufferSize = static_cast<UINT>(m_vertices.size() * sizeof(Vertex));
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
		IID_PPV_ARGS(&m_renderVertexBuffer)
	));

	// 버텍스 데이터 복사
	UINT8*		pVertexDataBegin;
	D3D12_RANGE readRange = {0, 0};
	ThrowIfFailed(m_renderVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, m_vertices.data(), vertexBufferSize);
	m_renderVertexBuffer->Unmap(0, nullptr);

	// 버텍스 버퍼 뷰 설정
	m_vertexBufferView.BufferLocation = m_renderVertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	// 인덱스 버퍼 생성
	if (!m_indices.empty())
	{
		const UINT indexBufferSize = static_cast<UINT>(m_indices.size() * sizeof(uint32_t));
		bufferDesc.Width = indexBufferSize;

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_renderIndexBuffer)
		));

		// 인덱스 데이터 복사
		UINT8* pIndexDataBegin;
		ThrowIfFailed(m_renderIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, m_indices.data(), indexBufferSize);
		m_renderIndexBuffer->Unmap(0, nullptr);

		// 인덱스 버퍼 뷰 설정
		m_indexBufferView.BufferLocation = m_renderIndexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView.SizeInBytes = indexBufferSize;
	}

	// 상수 버퍼 생성
	const UINT constantBufferSize = (sizeof(ConstantBuffer) + 255) & ~255;
	bufferDesc.Width = constantBufferSize;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_constantBuffer)
	));

	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));

	DEBUG_LOG_FMT(
		"[MeshComponent] Initialized for rendering: {} vertices, {} indices\n", GetVertexCount(), GetIndexCount()
	);
}

void MeshComponent::Render(
	ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection
)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
		return;

	// 메시 설정
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Transform 컴포넌트 가져오기
	auto&		 transform = myGameObject->GetTransform();
	DX::XMFLOAT3 pos = transform.GetPosition();
	DX::XMFLOAT3 rot = transform.GetRotation();

	// 월드 행렬 계산
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationY(rot.y);
	DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
	DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	DirectX::XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	DirectX::XMMATRIX mvp = worldMatrix * view * projection;

	// 상수 버퍼 업데이트
	DirectX::XMStoreFloat4x4(&m_constantBufferData.mvp, DirectX::XMMatrixTranspose(mvp));
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	// 메시 렌더링
	cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(static_cast<UINT>(GetIndexCount()), 1, 0, 0, 0);
}