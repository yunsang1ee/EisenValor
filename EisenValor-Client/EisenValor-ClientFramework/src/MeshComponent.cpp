#include "stdafxClientFramework.h"
#include "MeshComponent.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"
#include "GameObject.h"

void MeshComponent::SetMesh(
	const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, std::string_view name
)
{
	m_vertices = vertices;
	m_indices = indices;
	this->m_name = name.empty() ? GetOwner()->GetName() + "_Mesh" : std::string(name);
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