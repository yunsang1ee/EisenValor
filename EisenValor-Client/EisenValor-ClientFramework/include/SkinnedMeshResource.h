#pragma once
#include "IResource.h"
#include "AssetFormat.h"
#include "SkinnedMeshData.h"
#include "DxBuffer.h"

class SkinnedMeshResource : public ResourceBase<SkinnedMeshResource>
{
public:
	SkinnedMeshResource() = default;
	virtual ~SkinnedMeshResource() override = default;

	void SetMetadata(const EvAsset::Bounds& bounds, std::vector<EvAsset::SubMesh>&& subMeshes, uint32_t vertexCount,
					 uint32_t indexCount, uint32_t indexFormat, std::vector<EvAsset::Bone>&& bones,
					 std::vector<float>&& offsetMatrices, std::vector<EvAsset::Guid>&& materialGuids);
	void SetGPUResources(std::unique_ptr<DxBuffer>&& vb, std::unique_ptr<DxBuffer>&& ib);

	const EvAsset::Bounds&			   GetBounds() const { return m_bounds; }
	const std::vector<EvAsset::SubMesh>& GetSubMeshes() const { return m_subMeshes; }
	uint32_t						   GetVertexCount() const { return m_vertexCount; }
	uint32_t						   GetIndexCount() const { return m_indexCount; }
	uint32_t						   GetIndexFormat() const { return m_indexFormat; }
	const std::vector<EvAsset::Bone>&	GetBones() const { return m_bones; }
	const std::vector<float>&		   GetOffsetMatrices() const { return m_offsetMatrices; }
	const std::vector<EvAsset::Guid>&  GetDefaultMaterialGuids() const { return m_materialGuids; }

	DxBuffer* GetVertexBuffer() const { return m_vb.get(); }
	DxBuffer* GetIndexBuffer() const { return m_ib.get(); }

private:
	EvAsset::Bounds			   m_bounds{};
	std::vector<EvAsset::SubMesh>  m_subMeshes;
	uint32_t					   m_vertexCount = 0;
	uint32_t					   m_indexCount = 0;
	uint32_t					   m_indexFormat = 32;

	std::unique_ptr<DxBuffer> m_vb; // Vertex Buffer
	std::unique_ptr<DxBuffer> m_ib; // Index Buffer

	std::vector<EvAsset::Bone> m_bones;
	std::vector<float>		   m_offsetMatrices;
	std::vector<EvAsset::Guid> m_materialGuids;
};
