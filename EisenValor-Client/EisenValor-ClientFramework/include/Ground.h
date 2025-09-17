#pragma once
#include "DxCommon.h"
#include "Vertex.h"

class Ground
{
public:
	Ground() = default;
	~Ground() = default;

	void Initialize(ID3D12Device* device);
	void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

	void			BuildAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList);
	ID3D12Resource* GetBottomLevelAS() const { return m_bottomLevelAS.Get(); }

	// Geometry 데이터 접근용 (BLAS 빌드에 필요)
	const std::vector<Vertex>&	 GetVertices() const { return m_vertices; }
	const std::vector<uint16_t>& GetIndices() const { return m_indices; }
	ID3D12Resource*				 GetVertexBuffer() const { return m_vertexBuffer.Get(); }
	ID3D12Resource*				 GetIndexBuffer() const { return m_indexBuffer.Get(); }

	DirectX::XMMATRIX GetTransform() const;

	virtual RaytracingInstanceType GetRaytracingInstanceType() const { return RaytracingInstanceType::Ground; }

private:
	// 렌더링 리소스
	ComPtr<ID3D12Resource>	 m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource>	 m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW	 m_indexBufferView;

	// 상수 버퍼
	ComPtr<ID3D12Resource> m_constantBuffer;
	ConstantBuffer		   m_constantBufferData;
	UINT8*				   m_pCbvDataBegin = nullptr;

	// DXR 관련 리소스
	ComPtr<ID3D12Resource> m_bottomLevelAS;
	ComPtr<ID3D12Resource> m_scratchBuffer;

	std::vector<Vertex>	  m_vertices;
	std::vector<uint16_t> m_indices;

	// 크기 및 위치
	float			  m_width = 20.0f;
	float			  m_height = 0.2f;
	float			  m_depth = 20.0f;
	DirectX::XMFLOAT3 m_position = {0.0f, 0.0f, 0.0f};
};
