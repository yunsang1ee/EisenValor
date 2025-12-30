#pragma once
#include <IRenderPass.h>
#include <RenderDataSync.h>
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <DxTLAS.h>
#include <DxTexture.h>
#include <memory>

struct alignas(16) VertexPNU
{
	DX::XMFLOAT3 position;
	float		 pad_0;
	DX::XMFLOAT3 normal;
	float		 pad_1;
	DX::XMFLOAT2 uv;
	DX::XMFLOAT2 pad_2;

	VertexPNU(DX::XMFLOAT3 pos, DX::XMFLOAT3 nrm, DX::XMFLOAT2 tex)
		: position(pos), pad_0(0.0f), normal(nrm), pad_1(0.0f), uv(tex), pad_2{0.0f, 0.0f}
	{
	}
};
static_assert(sizeof(VertexPNU) % 16 == 0, "VertexPNU size must be multiple of 16 bytes");

struct alignas(16) PBRMaterial
{
	DX::XMFLOAT3 albedo;
	float		 metallic;
	float		 roughness;
	DX::XMFLOAT3 pad_0;
	DX::XMFLOAT3 emissive;
	float		 emissiveStrength;
};
static_assert(sizeof(PBRMaterial) % 16 == 0, "PBRMaterial size must be multiple of 16 bytes");

struct alignas(16) GeoInfo
{
	uint32_t vertexBase;
	uint32_t indexBase;
	uint32_t vertexCount;
	uint32_t indexCount;
};
static_assert(sizeof(GeoInfo) % 16 == 0, "GeoInfo size must be multiple of 16 bytes");

struct alignas(16) InstanceData
{
	DX::XMFLOAT4X4 worldMatrix;
	DX::XMFLOAT4X4 worldIT;
	uint32_t	   materialIndex;
	uint32_t	   geoIndex;
	uint32_t	   pad0;
	uint32_t	   pad1;
};
static_assert(sizeof(GeoInfo) % 16 == 0, "InstanceData size must be multiple of 16 bytes");

class DxrRenderPass : public IRenderPass
{
public:
	DxrRenderPass(uint32_t width, uint32_t height);
	~DxrRenderPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "DXR"; }

	DxTexture* GetOutputTexture() { return &m_raytracingOutput; }

private:
	void CreateRaytracingPipeline();
	void CreateRaytracingResources(uint32_t width, uint32_t height);
	void CollectRenderData(Scene* scene);
	void BuildAccelerationStructures();
	void CreateGeometryBuffers();

private:
	std::unique_ptr<DxRtPipelineState> m_rtPipeline;
	std::unique_ptr<DxRtPipelineState> m_ptPipeline;
	std::unique_ptr<DxRtShaderTable>   m_rtShaderTable;
	std::unique_ptr<DxRtShaderTable>   m_ptShaderTable;
	std::unique_ptr<DxTLAS>			   m_tlas;

	DxTexture m_raytracingOutput;

	struct MeshRenderData
	{
		RenderDataSync<VertexPNU>::Handle verticesHandle;
		RenderDataSync<uint32_t>::Handle  indicesHandle;
		RenderDataSync<GeoInfo>::Handle	  geoInfoHandle;
		RenderDataSync<uint32_t>::Handle  instGeoBaseHandle;
		uint32_t						  materialIndex = 0;

		size_t vertexCount = 0;
		size_t indexCount = 0;
	};
	std::unordered_map<uint64_t, MeshRenderData> m_meshDataMap;

	RenderDataSync<PBRMaterial> m_materials;   // t1: Material Buffer
	RenderDataSync<VertexPNU>	m_vertices;	   // t2: Vertex Buffer
	RenderDataSync<uint32_t>	m_indices;	   // t3: Index Buffer
	RenderDataSync<GeoInfo>		m_geoInfos;	   // t4: GeoInfo Buffer
	RenderDataSync<uint32_t>	m_instGeoBase; // t5: InstGeoBase Buffer

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	bool m_initialized = false;
	bool m_needsRebuild = false;
	bool m_usePathTracing = false;
};
