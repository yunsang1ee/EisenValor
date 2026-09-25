#pragma once
#include <IRenderPass.h>
#include "RaytracingControls.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include <DxTexture.h>
#include <RenderDataPolicy.h>
#include <DirectXMath.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "RenderData/InstanceRenderData.h"
#include "RenderData/MaterialRenderData.h"
#include "RenderData/GeoTableRenderData.h"
#include "RenderData/RestirLightRenderData.h"
#include "RenderData/StaticSceneRenderData.h"
#include "RenderData/TlasRenderData.h"
namespace EvAsset
{
struct SubMesh;
}
class Scene;
class DxFrameResource;
class RenderContext;
class MaterialResource;
enum class RenderMobility : uint8_t;

class RaytracingPreparePass : public IRenderPass
{
public:
	RaytracingPreparePass(uint32_t width, uint32_t height);
	void				   Initialize() override;
	void				   Release() override;
	void				   DeclareRenderData(RenderContext* renderContext) override;
	void				   Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void				   OnResize(uint32_t width, uint32_t height) override;
	RenderResolutionDomain GetResolutionDomain() const override { return RenderResolutionDomain::Render; }
	const char*			   GetName() const override { return "RaytracingPrepare"; }
	RaytracingControls&	   GetControls() { return m_controls; }

private:
	void BeginFrame(uint32_t frameIndex, Scene* scene);
	void Prepare(DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition, bool buildInstanceLookup);
	void Publish(RenderContext* renderContext);
	void CommitRenderedFrame();
	void CreateOutputResources(uint32_t width, uint32_t height);
	uint64_t BuildHistorySignature(uint64_t generation) const;
	uint32_t RegisterMaterial(MaterialResource* material, uint32_t ownerId, uint32_t materialSlot, uint32_t frameIndex);
	void	 RegisterGeometry(
			const EvAsset::SubMesh& subMesh,
			MaterialResource*		material,
			uint32_t				materialIndex,
			uint32_t				stableGeometryId,
			uint32_t				instanceIndex,
			uint32_t				subMeshIndex,
			uint32_t				frameIndex
		);
	void	 BeginInstanceMotionFrame(Scene* scene);
	void	 CommitInstanceMotionFrame(uint32_t frameIndex);
	void	 PrepareRenderData(DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition);
	uint32_t RegisterTerrainSurface(MaterialRenderData* materialData, MaterialResource* material) const;

	void CollectMeshData(
		Scene*						 scene,
		std::vector<DxTLASInstance>& tlasInstances,
		uint32_t					 frameIndex,
		RenderMobility				 mobility,
		bool&						 allResourcesReady
	);
	void CollectSkinnedMeshData(
		Scene*						 scene,
		ID3D12GraphicsCommandList4*	 cmdList,
		std::vector<DxTLASInstance>& tlasInstances,
		uint32_t					 frameIndex,
		bool&						 hasAnimatedInstances,
		uint32_t&					 animatedBlasCount
	);

	void RegisterInstanceIdLookup(uint32_t ownerId, uint32_t instanceIndex);
	void ApplyInstanceMotionHistory(
		InstanceData& instance, bool usePreviousSkinnedVertices, uint32_t previousVertexBufferIndex
	);

	RaytracingControls								  m_controls;
	Transient<RaytracingFrameRenderData>			  m_frameData;
	FrameBuffered<RaytracingOutputRenderData, 3>	  m_outputData;
	uint32_t										  m_width;
	uint32_t										  m_height;
	uint32_t										  m_frameSeed = 0;
	bool											  m_initialized = false;
	FrameBuffered<TlasRenderData, 3>				  m_tlas;
	FrameBuffered<InstanceRenderData, 3>			  m_instanceData;
	FrameBuffered<MaterialRenderData, 3>			  m_materialData;
	FrameBuffered<GeoTableRenderData, 3>			  m_geoTableData;
	FrameBuffered<RestirLightRenderData, 3>			  m_restirLightData;
	Persistent<StaticSceneRenderData>				  m_staticSceneData;
	std::vector<DxTLASInstance>						  m_tlasInstancesScratch;
	std::vector<uint32_t>							  m_instanceIdLookupScratch;
	std::unordered_map<uint64_t, DirectX::XMFLOAT4X4> m_previousInstanceWorldMatrices;
	std::unordered_map<uint64_t, DirectX::XMFLOAT4X4> m_currentInstanceWorldMatrices;
	Scene*											  m_instanceMotionScene = nullptr;
	uint32_t										  m_previousInstanceFrameIndex = 0;
	bool											  m_hasPreviousInstanceFrame = false;
	uint32_t										  m_lastAnimatedBlasCount = 0;
	ComPtr<ID3D12Device5>							  m_device5;
	DirectX::XMFLOAT4X4								  m_previousViewProj = {};
	bool											  m_hasPreviousViewProj = false;
};
