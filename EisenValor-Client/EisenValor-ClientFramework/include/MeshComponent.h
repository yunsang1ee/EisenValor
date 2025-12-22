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
	static constexpr const char* GetStaticTypeName() { return "MeshComponent"; }

	MeshComponent() = default;
	~MeshComponent() override = default;

	// 메시 데이터 설정
	void SetMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, std::string_view name = "");

	// Raytracing용 BLAS 빌드
	void BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, class DxUploadHeap* uploadHeap);

	// 일반 렌더링용 초기화
	void Initialize(ID3D12Device* device);

	// 렌더링
	void Render(ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);

	// Raytracing 접근자
	DxBLAS*					  GetBLAS() const { return m_blas.get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetVertexBufferGPU() const { return m_vertexBufferGPU; }
	D3D12_GPU_VIRTUAL_ADDRESS GetIndexBufferGPU() const { return m_indexBufferGPU; }

	// 메시 정보
	uint32_t					 GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
	uint32_t					 GetIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }
	const std::vector<Vertex>&	 GetVertices() const { return m_vertices; }
	const std::vector<uint32_t>& GetIndices() const { return m_indices; }

	// Scale 설정
	void		SetScale(const Vec3& scale) { m_scale = scale; }
	void		SetScale(float x, float y, float z) { m_scale = {x, y, z}; }
	void		SetUniformScale(float scale) { m_scale = {scale, scale, scale}; }
	const Vec3& GetScale() const { return m_scale; }

	// Color 설정
	void		SetColor(const Vec3& color) { m_color = Vec3{color.x, color.y, color.z}; }
	void		SetColor(float r, float g, float b) { m_color = Vec3{r, g, b}; }
	const Vec3& GetColor() const { return m_color; }

private:
	// 헬퍼 함수: GameObject 포인터 얻기
	class GameObject* GetGameObject() const;

	std::string			  m_name;
	std::vector<Vertex>	  m_vertices;
	std::vector<uint32_t> m_indices;

	// Raytracing용
	std::unique_ptr<DxBLAS>	  m_blas;
	ComPtr<ID3D12Resource>	  m_vertexBuffer; // Raytracing용 (DEFAULT heap)
	ComPtr<ID3D12Resource>	  m_indexBuffer;  // Raytracing용 (DEFAULT heap)
	D3D12_GPU_VIRTUAL_ADDRESS m_vertexBufferGPU = 0;
	D3D12_GPU_VIRTUAL_ADDRESS m_indexBufferGPU = 0;

	// 일반 렌더링용
	ComPtr<ID3D12Resource>	 m_renderVertexBuffer; // 렌더링용 (UPLOAD heap)
	ComPtr<ID3D12Resource>	 m_renderIndexBuffer;  // 렌더링용 (UPLOAD heap)
	ComPtr<ID3D12Resource>	 m_constantBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW	 m_indexBufferView = {};
	ConstantBuffer			 m_constantBufferData = {};
	UINT8*					 m_pCbvDataBegin = nullptr;

	// 렌더링 설정
	Vec3 m_scale{1.0f, 1.0f, 1.0f};
	Vec3 m_color{1.0f, 0.0f, 0.0f};
};