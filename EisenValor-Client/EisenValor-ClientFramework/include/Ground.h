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

	// 크기 및 위치
	float			  m_width = 20.0f;
	float			  m_height = 0.2f;
	float			  m_depth = 20.0f;
	DirectX::XMFLOAT3 m_position = {0.0f, 0.0f, 0.0f};
};
