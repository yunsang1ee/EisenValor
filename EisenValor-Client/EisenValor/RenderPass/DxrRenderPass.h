#pragma once
#include <IRenderPass.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <DxTexture.h>
#include <RenderDataPolicy.h>
#include <DirectXMath.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "RenderData/InstanceRenderData.h"
#include "RenderData/MaterialRenderData.h"
#include "RenderData/GeoTableRenderData.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include "RenderData/RestirCandidateRenderData.h"
#include "RenderData/RestirLightRenderData.h"
#include "RenderData/StaticSceneRenderData.h"
#include "RenderData/TlasRenderData.h"

class MeshComponent;
class MeshResource;
class MaterialResource;
class DxBLAS;
class DxBuffer;
class Scene;
enum class RenderMobility : uint8_t;

class DxrRenderPass : public IRenderPass
{
public:
	DxrRenderPass(uint32_t width, uint32_t height);
	~DxrRenderPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		DeclareRenderData(RenderContext* renderContext) override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	RenderResolutionDomain GetResolutionDomain() const override { return RenderResolutionDomain::Render; }
	const char* GetName() const override { return "DXR"; }

private:
	void						CreateRaytracingPipeline();
	void						CreateRaytracingResources(uint32_t width, uint32_t height);
	ComPtr<ID3D12RootSignature> BuildDxrGlobalRootSignature(
		bool enableRestirCandidateRoot, std::string_view name
	) const;
	std::unique_ptr<DxRtPipelineState> BuildDxrPipeline(
		const std::wstring& shaderPath,
		uint32_t			maxRecursionDepth,
		uint32_t			maxPayloadSizeBytes,
		bool				enableRestirCandidateRoot
	);
	std::unique_ptr<DxRtShaderTable> BuildDxrShaderTable(
		const DxRtPipelineState* pipelineState, std::string_view name
	) const;

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
		bool&						 hasAnimatedInstances
	);

	void RegisterInstanceIdLookup(uint32_t ownerId, uint32_t instanceIndex);

private:
	std::unique_ptr<DxRtPipelineState> m_rtPipeline;
	std::unique_ptr<DxRtPipelineState> m_ptPipeline;
	std::unique_ptr<DxRtPipelineState> m_restirCandidatePipeline;
	std::unique_ptr<DxRtShaderTable>   m_rtShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_ptShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_restirCandidateShaderTable;

	FrameBuffered<TlasRenderData, 3> m_tlas;

	FrameBuffered<RaytracingOutputRenderData, 3> m_outputData;
	Transient<RestirCandidateRenderData>		 m_restirCandidateData;

	FrameBuffered<InstanceRenderData, 3>	m_instanceData;
	FrameBuffered<MaterialRenderData, 3>	m_materialData;
	FrameBuffered<GeoTableRenderData, 3>	m_geoTableData;
	FrameBuffered<RestirLightRenderData, 3> m_restirLightData;

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	ComPtr<ID3D12Device5> m_device5;
	DirectX::XMFLOAT4X4	  m_previousViewProj = {};

	bool	 m_initialized = false;
	bool	 m_usePathTracing = false;
	bool	 m_useRestirPT = false;
	bool	 m_usePhysicalEmissionView = false;
	bool	 m_useDayEnvironment = false;
	bool	 m_hasPreviousViewProj = false;
	uint32_t m_raytracingFrameSeed = 0;
	uint64_t m_restirHistoryGeneration = 1;

	Persistent<StaticSceneRenderData> m_staticSceneData;
	std::vector<DxTLASInstance>		  m_tlasInstancesScratch;
	std::vector<uint32_t>			  m_instanceIdLookupScratch;
};
