#include "stdafxClientFramework.h"
#include "SkinnedMeshComponent.h"

void SkinnedMeshComponent::SetMesh(const std::vector<SkinnedVertex>& vertices, const std::vector<uint32_t>& indices)
{
    m_vertices = vertices;
    m_indices = indices;
}

void SkinnedMeshComponent::SetSkeleton(const std::vector<SkinnedBone>& bones, const std::vector<float>& offsetMatrices)
{
    m_bones = bones;
    m_offsetMatrices = offsetMatrices;
}
