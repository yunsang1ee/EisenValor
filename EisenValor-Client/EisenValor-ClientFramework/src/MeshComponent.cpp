#include "stdafxClientFramework.h"
#include "MeshComponent.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"

void MeshComponent::SetMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	m_vertices = vertices;
	m_indices = indices;
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
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)
	));

	m_vertexBuffer->SetName(L"VertexBuffer");
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
			&defaultHeap, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(&m_indexBuffer)
		));

		m_indexBuffer->SetName(L"IndexBuffer");

		cmdList->CopyBufferRegion(m_indexBuffer.Get(), 0, uploadHeap->GetResource(), ibUpload.offset, ibSize);

		barrier = DxUtils::CreateTransitionBarrier(
			m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier);

		m_indexBufferGPU = m_indexBuffer->GetGPUVirtualAddress();
	}

	m_blas = std::make_unique<DxBLAS>();
	m_blas->Build(
		device, cmdList, m_vertexBufferGPU, GetVertexCount(), sizeof(Vertex), m_indexBufferGPU, GetIndexCount(),
		false
	);

	DEBUG_LOG_FMT("[MeshComponent] Built BLAS: {} vertices, {} indices\n", GetVertexCount(), GetIndexCount());
}