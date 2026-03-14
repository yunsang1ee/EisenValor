#pragma once
#include "IResource.h"
#include <vector>
#include <memory>

class DxBuffer;
class DxBLAS;

namespace EvAsset
{
struct SubMesh;
struct Bounds;
} // namespace EvAsset

class MeshResource final : public ResourceBase<MeshResource>
{
public:
	static constexpr const char* GetStaticResourceName() { return "MeshResource"; }

	MeshResource();
	~MeshResource() override;

	DxBuffer* GetVertexBuffer() const { return m_vertexBuffer.get(); }
	DxBuffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
	DxBLAS*	  GetBLAS() const { return m_blas.get(); }

	const EvAsset::Bounds&				 GetBounds() const { return m_bounds; }
	const std::vector<EvAsset::SubMesh>& GetSubMeshes() const { return m_subMeshes; }
	const std::vector<EvAsset::Guid>&	 GetDefaultMaterialGuids() const { return m_defaultMaterialGuids; }

	uint32_t GetVertexCount() const { return m_vertexCount; }
	uint32_t GetIndexCount() const { return m_indexCount; }
	uint32_t GetIndexFormat() const { return m_indexFormat; }

	void SetGPUResources(std::unique_ptr<DxBuffer> vb, std::unique_ptr<DxBuffer> ib, std::unique_ptr<DxBLAS> blas);

	void SetMetadata(
		const EvAsset::Bounds&			bounds,
		std::vector<EvAsset::SubMesh>&& subMeshes,
		std::vector<EvAsset::Guid>&&	defaultMaterialGuids,
		uint32_t						vCount,
		uint32_t						iCount,
		uint32_t						iFormat
	);

private:
	std::unique_ptr<DxBuffer> m_vertexBuffer;
	std::unique_ptr<DxBuffer> m_indexBuffer;
	std::unique_ptr<DxBLAS>	  m_blas;

	std::vector<EvAsset::SubMesh> m_subMeshes;
	std::vector<EvAsset::Guid>	  m_defaultMaterialGuids;
	EvAsset::Bounds				  m_bounds{};

	uint32_t m_vertexCount = 0;
	uint32_t m_indexCount = 0;
	uint32_t m_indexFormat = 32;
};
