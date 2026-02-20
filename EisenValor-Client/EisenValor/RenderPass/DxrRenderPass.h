#pragma once
#include <IRenderPass.h>
#include <RenderDataSync.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <DxTLAS.h>
#include <DxTexture.h>
#include <memory>
#include "AssetFormat.h"

class MeshComponent;
class MeshResource;
class MaterialResource;
class DxBLAS;

struct alignas(16) GeoInfo
{
	uint32_t vertexBase;
	uint32_t indexBase;
	uint32_t materialIdx;
	uint32_t pad0;
};

struct alignas(16) InstanceData
{
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldIT;
	uint32_t			vertexBufferIdx;
	uint32_t			indexBufferIdx;
	uint32_t			geoInfoBaseIdx;
	uint32_t			instanceID;
};
static_assert(sizeof(InstanceData) % 16 == 0, "InstanceData size must be multiple of 16 bytes");

struct alignas(16) MaterialGPUData
{
	DirectX::XMFLOAT4 albedo;
	float			  roughness;
	float			  metallic;
	uint32_t		  shadingModel;
	uint32_t		  materialFlags;

	uint32_t albedoTextureIdx;
	uint32_t normalTextureIdx;
	uint32_t ormTextureIdx;
	uint32_t pad0;
};
static_assert(sizeof(MaterialGPUData) % 16 == 0, "MaterialGPUData size must be multiple of 16 bytes");

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

	DxTexture* GetOutputTexture() { return &m_raytracingOutput; }

private:
	void CreateRaytracingPipeline();
	void CreateRaytracingResources(uint32_t width, uint32_t height);
		
	void CollectRenderData(Scene* scene);
	void BuildAccelerationStructures(DxFrameResource* frame, Scene* scene);

private:
	std::unique_ptr<DxRtPipelineState> m_rtLitePipeline;
	std::unique_ptr<DxRtPipelineState> m_rtPipeline;
	std::unique_ptr<DxRtPipelineState> m_ptPipeline;
	std::unique_ptr<DxRtShaderTable>   m_rtLiteShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_rtShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_ptShaderTable;
	std::unique_ptr<DxTLAS>			   m_tlas;

	DxTexture m_raytracingOutput;

	std::unordered_map<EvAsset::Guid, std::unique_ptr<DxBLAS>, EvAsset::GuidHash> m_blasCache;

	RenderDataSync<InstanceData>	m_instanceBuffer;
	RenderDataSync<MaterialGPUData> m_materialConstants;
	RenderDataSync<GeoInfo>         m_geoTable;

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	bool m_initialized = false;
	bool m_needsRebuild = false;
	bool m_usePathTracing = false;
	bool m_useLiteRT = false;
};
