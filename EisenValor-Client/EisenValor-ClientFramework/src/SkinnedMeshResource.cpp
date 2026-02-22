#include "stdafxClientFramework.h"
#include "SkinnedMeshResource.h"

void SkinnedMeshResource::SetGPUResources(std::unique_ptr<DxBuffer>&& vb, std::unique_ptr<DxBuffer>&& ib)
{
	m_vb = std::move(vb);
	m_ib = std::move(ib);
}
void SkinnedMeshResource::SetMetadata(const EvAsset::Bounds& bounds, std::vector<EvAsset::SubMesh>&& subMeshes,
									  uint32_t vertexCount, uint32_t indexCount, uint32_t indexFormat,
									  std::vector<EvAsset::Bone>&& bones, std::vector<float>&& offsetMatrices,
									  std::vector<EvAsset::Guid>&& materialGuids)
{
	m_bounds = bounds;
	m_subMeshes = std::move(subMeshes);
	m_vertexCount = vertexCount;
	m_indexCount = indexCount;
	m_indexFormat = indexFormat;
	m_bones = std::move(bones);
	m_offsetMatrices = std::move(offsetMatrices);
	m_materialGuids = std::move(materialGuids);
}

