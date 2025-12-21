#pragma once
#include "stdafxClientFramework.h"
#include "IComponent.h"
#include "DxCommon.h"
#include "Vertex.h"

class MeshRenderer : public IComponent
{
private:
	// DirectX 리소스들
	ComPtr<ID3D12Resource>	 m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource>	 m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW	 m_indexBufferView;
	ComPtr<ID3D12Resource>	 m_constantBuffer;
	ConstantBuffer			 m_constantBufferData;
	UINT8*					 m_pCbvDataBegin = nullptr;

	Vec3					 m_scale{1.0f, 1.0f, 1.0f}; // 크기 설정
	Vec3					 m_color{1.0f, 0.0f, 0.0f}; // 색상 설정

public:
	MeshRenderer() = default;
	virtual ~MeshRenderer() = default;

	// IComponent 인터페이스
	virtual ComponentFlags GetFlags() const override { return ComponentFlags::Renderable; }

	// 함수 선언
	void Initialize(ID3D12Device* device);
	void Render(ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);
	void SetScale(const Vec3& scale) { m_scale = scale; }
	void SetScale(float x, float y, float z) { m_scale = {x, y, z}; }
	void SetUniformScale(float scale) { m_scale = {scale, scale, scale}; }
	const Vec3& GetScale() const { return m_scale; }

	void		SetColor(const Vec3& color) { m_color = color; }
	void		SetColor(float r, float g, float b) { m_color = {r, g, b}; }
	const Vec3& GetColor() const { return m_color; }
};