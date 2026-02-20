#pragma once
#include "IComponent.h"
#include "Vertex.h"

struct SkinnedBone
{
    uint64_t nameHash;
    int32_t  parentIndex;
    DirectX::XMFLOAT3 restPos;
    DirectX::XMFLOAT4 restRot;
    DirectX::XMFLOAT3 restScale;
};

class SkinnedMeshComponent : public ComponentBase<SkinnedMeshComponent>
{
public:
    static constexpr const char* GetStaticTypeName() { return "SkinnedMeshComponent"; }

    SkinnedMeshComponent() = default;
    ~SkinnedMeshComponent() override = default;

    // 데이터 설정
    void SetMesh(const std::vector<SkinnedVertex>& vertices, const std::vector<uint32_t>& indices);
    void SetSkeleton(const std::vector<SkinnedBone>& bones, const std::vector<float>& offsetMatrices);

    // 접근자
    uint32_t                           GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
    uint32_t                           GetIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }
    const std::vector<SkinnedVertex>&  GetVertices() const { return m_vertices; }
    const std::vector<uint32_t>&       GetIndices() const { return m_indices; }
    const std::vector<SkinnedBone>&    GetBones() const { return m_bones; }
    const std::vector<float>&          GetOffsetMatrices() const { return m_offsetMatrices; }

    bool IsValid() const { return !m_vertices.empty() && !m_bones.empty(); }

private:
    std::vector<SkinnedVertex> m_vertices;
    std::vector<uint32_t>      m_indices;
    std::vector<SkinnedBone>   m_bones;
    std::vector<float>         m_offsetMatrices;
};
