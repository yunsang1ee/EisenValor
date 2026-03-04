#include "stdafxClientFramework.h"
#include "MeshResource.h"
#include "DxBuffer.h"
#include "DxBLAS.h"
#include "DxUtils.h"

MeshResource::MeshResource() = default;
MeshResource::~MeshResource() = default;

void MeshResource::SetGPUResources(std::unique_ptr<DxBuffer> vb, std::unique_ptr<DxBuffer> ib, std::unique_ptr<DxBLAS> blas)
{
	m_vertexBuffer = std::move(vb);
	m_indexBuffer = std::move(ib);
	m_blas = std::move(blas);
}

void MeshResource::SetMetadata(
	const EvAsset::Bounds&			bounds,
	std::vector<EvAsset::SubMesh>&& subMeshes,
	std::vector<EvAsset::Guid>&&	defaultMaterialGuids,
	uint32_t						vCount,
	uint32_t						iCount,
	uint32_t						iFormat
)
{
	m_bounds = bounds;
	m_subMeshes = std::move(subMeshes);
	m_defaultMaterialGuids = std::move(defaultMaterialGuids);
	m_vertexCount = vCount;
	m_indexCount = iCount;
	m_indexFormat = iFormat;
}
