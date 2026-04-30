#pragma once
#include <IRenderPass.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <DxTLAS.h>
#include <DxTexture.h>
#include <memory>
#include "AssetFormat.h"
#include "RenderData/InstanceRenderData.h"
#include "RenderData/MaterialRenderData.h"
#include "RenderData/GeoTableRenderData.h"
#include "RenderData/RaytracingOutputRenderData.h"

class MeshComponent;
class MeshResource;
class MaterialResource;
class DxBLAS;

class DxrRenderPass : public IRenderPass
{
public:
	DxrRenderPass(uint32_t width, uint32_t height);
	~DxrRenderPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "DXR"; }

private:
	void CreateRaytracingPipeline();
	void CreateRaytracingResources(uint32_t width, uint32_t height);

	void PrepareRenderData(DxFrameResource* frame, Scene* scene);

	void CollectStaticMeshData(
		Scene*										  scene,
		ID3D12GraphicsCommandList4*					  cmdList,
		std::vector<DxTLASInstance>&				  tlasInstances,
		uint32_t									  frameIndex
	);
	void CollectSkinnedMeshData(
		Scene*										  scene,
		ID3D12GraphicsCommandList4*					  cmdList,
		std::vector<DxTLASInstance>&				  tlasInstances,
		uint32_t									  frameIndex,
		bool&										  hasAnimatedInstances
	);

private:
	std::unique_ptr<DxRtPipelineState> m_rtLitePipeline;
	std::unique_ptr<DxRtPipelineState> m_rtPipeline;
	std::unique_ptr<DxRtPipelineState> m_ptPipeline;
	std::unique_ptr<DxRtShaderTable>   m_rtLiteShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_rtShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_ptShaderTable;
	
	std::unique_ptr<DxTLAS>			   m_tlas[3];

	std::shared_ptr<RaytracingOutputRenderData> m_outputData[3];

	std::shared_ptr<InstanceRenderData> m_instanceData[3];
	std::shared_ptr<MaterialRenderData> m_materialData[3];
	std::shared_ptr<GeoTableRenderData> m_geoTableData[3];
	uint32_t m_lastTlasInstanceCount[3] = {};
	uint32_t m_tlasStableFrameCount[3] = {};
	uint64_t m_lastTlasTopologyHash[3] = {};

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	ComPtr<ID3D12Device5> m_device5;

	bool m_initialized = false;
	bool m_usePathTracing = false;
	bool m_useLiteRT = false;
};
