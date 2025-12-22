#pragma once
#include "IComponent.h"

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

	// 순수 데이터 접근자
	uint32_t					 GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
	uint32_t					 GetIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }
	const std::vector<Vertex>&	 GetVertices() const { return m_vertices; }
	const std::vector<uint32_t>& GetIndices() const { return m_indices; }
	const std::string&			 GetName() const { return m_name; }

	// 메시 데이터 유효성 검사
	bool IsValid() const { return !m_vertices.empty(); }

private:
	std::string			  m_name;
	std::vector<Vertex>	  m_vertices;
	std::vector<uint32_t> m_indices;
};