#include "stdafxClientFramework.h"
#include "MeshResource.h"
#include "DxBuffer.h"
#include "DxBLAS.h"
#include "DxUtils.h"
#include "TriangleSceneQuery.h"

void MeshResource::SetQueryGeometry(std::span<const EvAsset::Vertex> vertices, std::vector<uint32_t> indices)
{
	std::vector<DirectX::XMFLOAT3> positions;
	positions.reserve(vertices.size());
	for (const auto& vertex : vertices)
	{
		positions.emplace_back(vertex.position[0], vertex.position[1], vertex.position[2]);
	}
	m_queryGeometry = std::make_unique<TriangleSceneQuery>();
	m_queryGeometry->SetGeometry(std::move(positions), std::move(indices));
}

MeshResource::MeshResource() = default;
MeshResource::~MeshResource() = default;

void MeshResource::SetGPUResources(std::unique_ptr<DxBuffer> vb, std::unique_ptr<DxBuffer> ib, std::unique_ptr<DxBLAS> blas)
{
	m_vertexBuffer = std::move(vb);
	m_indexBuffer = std::move(ib);
	m_blas = std::move(blas);
	SetReady(m_vertexBuffer != nullptr && m_indexBuffer != nullptr && m_blas != nullptr);
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
