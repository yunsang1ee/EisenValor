#pragma once
#include <IRenderPass.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <DxTLAS.h>
#include <DxTexture.h>
#include <RenderDataPolicy.h>
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "AssetFormat.h"
#include "RenderData/InstanceRenderData.h"
#include "RenderData/MaterialRenderData.h"
#include "RenderData/GeoTableRenderData.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include "RenderData/RestirFinalReservoirRenderData.h"
#include "RenderData/RestirPrimaryHitCurrentRenderData.h"
#include "RenderData/RestirReservoirInitialRenderData.h"

class MeshComponent;
class MeshResource;
class MaterialResource;
class DxBLAS;
class DxBuffer;

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
	const char* GetName() const override { return "DXR"; }

private:
	void CreateRaytracingPipeline();
	void CreateRaytracingResources(uint32_t width, uint32_t height);
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

	void PrepareRenderData(DxFrameResource* frame, Scene* scene, const DX::XMFLOAT3* cameraPosition);
	uint32_t RegisterTerrainSurface(MaterialRenderData* materialData, MaterialResource* material) const;

	void CollectStaticMeshData(
		Scene*						 scene,
		ID3D12GraphicsCommandList4*	 cmdList,
		std::vector<DxTLASInstance>& tlasInstances,
		uint32_t					 frameIndex
	);
	void CollectSkinnedMeshData(
		Scene*						 scene,
		ID3D12GraphicsCommandList4*	 cmdList,
		std::vector<DxTLASInstance>& tlasInstances,
		uint32_t					 frameIndex,
		bool&						 hasAnimatedInstances
	);

private:
	std::unique_ptr<DxRtPipelineState> m_rtPipeline;
	std::unique_ptr<DxRtPipelineState> m_ptPipeline;
	std::unique_ptr<DxRtPipelineState> m_restirCandidatePipeline;
	std::unique_ptr<DxRtShaderTable>   m_rtShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_ptShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_restirCandidateShaderTable;

	std::unique_ptr<DxTLAS> m_tlas[3];

	FrameBuffered<RaytracingOutputRenderData, 3> m_outputData;
	Transient<RestirPrimaryHitCurrentRenderData> m_restirPrimaryHitCurrentData;
	Transient<RestirReservoirInitialRenderData>	 m_restirReservoirInitialData;
	Transient<RestirFinalReservoirRenderData>	 m_restirFinalReservoirData;

	FrameBuffered<InstanceRenderData, 3> m_instanceData;
	FrameBuffered<MaterialRenderData, 3> m_materialData;
	FrameBuffered<GeoTableRenderData, 3> m_geoTableData;
	uint32_t							 m_lastTlasInstanceCount[3] = {};
	uint32_t							 m_tlasStableFrameCount[3] = {};
	uint64_t							 m_lastTlasTopologyHash[3] = {};

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	ComPtr<ID3D12Device5> m_device5;

	bool	 m_initialized = false;
	bool	 m_usePathTracing = false;
	bool	 m_useRestirPT = false;
	bool	 m_usePhysicalEmissionView = false;
	bool	 m_useDayEnvironment = false;
	uint32_t m_raytracingFrameSeed = 0;
};
