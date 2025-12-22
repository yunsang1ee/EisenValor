#pragma once
#include "IComponent.h"
#include "DxBLAS.h"

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT4 color;
};

class MeshComponent : public ComponentBase<MeshComponent>
{
public:
	MeshComponent() = default;
	~MeshComponent() = default;

	void SetMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, std::string_view name = "");

	void BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, class DxUploadHeap* uploadHeap);

	DxBLAS*					  GetBLAS() const { return m_blas.get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetVertexBufferGPU() const { return m_vertexBufferGPU; }
	D3D12_GPU_VIRTUAL_ADDRESS GetIndexBufferGPU() const { return m_indexBufferGPU; }
	uint32_t				  GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
	uint32_t				  GetIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }

	const std::vector<Vertex>&	 GetVertices() const { return m_vertices; }
	const std::vector<uint32_t>& GetIndices() const { return m_indices; }

private:
	std::string			  m_name;
	std::vector<Vertex>	  m_vertices;
	std::vector<uint32_t> m_indices;

	std::unique_ptr<DxBLAS> m_blas;

	ComPtr<ID3D12Resource>	  m_vertexBuffer;
	ComPtr<ID3D12Resource>	  m_indexBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS m_vertexBufferGPU = 0;
	D3D12_GPU_VIRTUAL_ADDRESS m_indexBufferGPU = 0;
};
