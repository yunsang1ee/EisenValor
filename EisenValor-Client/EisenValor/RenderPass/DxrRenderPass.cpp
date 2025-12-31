#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include <Scene.h>
#include <DxFrameResource.h>
#include <DxDeviceGlobal.h>
#include <DxDescriptorHeapGlobal.h>
#include <DxCommandQueueGlobal.h>
#include <InputGlobal.h>
#include <unordered_set>
#include <DenseList.h>

DxrRenderPass::DxrRenderPass(uint32_t width, uint32_t height) : m_width(width), m_height(height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Constructor: {}x{}\n", width, height);
}

void DxrRenderPass::Initialize()
{
	DEBUG_LOG_FMT("[DxrRenderPass] Initializing with resolution: {}x{}\n", m_width, m_height);

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	DEBUG_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	m_vertices.Clear();
	m_indices.Clear();
	m_materials.Clear();
	m_geoInfos.Clear();
	m_instGeoBase.Clear();

	m_initialized = false;
	DEBUG_LOG_FMT("[DxrRenderPass] Released\n");
}

void DxrRenderPass::OnResize(uint32_t width, uint32_t height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Resizing: {}x{} -> {}x{}\n", m_width, m_height, width, height);

	m_width = width;
	m_height = height;

	CreateRaytracingResources(m_width, m_height);
}

void DxrRenderPass::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	if (m_raytracingOutput.HasUAV(0))
	{
		auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
		auto  fenceValue = commandQueue.GetCurrentFenceValue();
		m_raytracingOutput.ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
	}

	m_raytracingOutput.Initialize(
		device.GetDevice(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "DxrRenderPass_Output"
	);
	m_raytracingOutput.CreateUAV(device.GetDevice(), descHeap, 0);

	DEBUG_LOG_FMT(
		"[DxrRenderPass] Raytracing output created: {}x{}, UAV Index={}\n", width, height,
		m_raytracingOutput.GetUAVIndex(0)
	);
}

void DxrRenderPass::CollectRenderData(Scene* scene) 
{
	auto* meshStorage = scene->GetStorage<MeshComponent>();
	if (!meshStorage)
		return;

	std::unordered_set<uint64_t> activeMeshes;

	uint32_t currentVertexBase = 0;
	uint32_t currentIndexBase = 0;

	for (MeshComponent& mesh : meshStorage->GetList()) 
	{
		uint64_t meshHandleValue = mesh.GetHandle().GetValue();
		activeMeshes.insert(meshHandleValue);

		auto& vertices = mesh.GetVertices();
		auto& indices = mesh.GetIndices();

		auto iter = m_meshDataMap.find(meshHandleValue);

		if (iter != m_meshDataMap.end())
		{
			MeshRenderData renderData;
			for (auto& v : vertices)
			{

			}
		}
	}
}

void DxrRenderPass::CreateRaytracingPipeline()
{
	auto&				  device = GLOBAL(DxDeviceGlobal);
	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));

	m_rtPipeline = std::make_unique<DxRtPipelineState>();
	m_rtPipeline->Create(device5.Get(), L"Resource/Shader/RaytracingLibrary.hlsl", 4);
	m_rtShaderTable = std::make_unique<DxRtShaderTable>();
	m_rtShaderTable->Build(device5.Get(), m_rtPipeline.get(), 1);

	m_ptPipeline = std::make_unique<DxRtPipelineState>();
	m_ptPipeline->Create(device5.Get(), L"Resource/Shader/RaytracingLibraryPT.hlsl", 19);
	m_ptShaderTable = std::make_unique<DxRtShaderTable>();
	m_ptShaderTable->Build(device5.Get(), m_ptPipeline.get(), 1);

	DEBUG_LOG_FMT("[GameFramework] Raytracing pipeline created\n");
}

void DxrRenderPass::Execute(DxFrameResource* frame, Scene* scene)
{
	if (!m_initialized || !scene)
		return;

	// 전처리
	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
		DEBUG_LOG_FMT("[GameFramework] Switched to {}\n", m_usePathTracing ? "Path Tracing" : "Ray Tracing");
	}

	// 데이터 동기화
	auto& device = GLOBAL(DxDeviceGlobal);
	auto* context = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();

	CollectRenderData(scene);

	m_vertices.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_indices.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_materials.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_geoInfos.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_instGeoBase.SyncToGPU(device.GetDevice(), *context, *uploadHeap);

	if (m_needsRebuild)
	{
		BuildAccelerationStructures();
		m_needsRebuild = false;
	}

	// DXR 실행
	auto*							   cmdList = context->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	if (m_usePathTracing)
	{
		cmdList4->SetPipelineState1(m_ptPipeline->GetStateObject());
		cmdList4->SetComputeRootSignature(m_ptPipeline->GetGlobalRootSignature());
	}
	else
	{
		cmdList4->SetPipelineState1(m_rtPipeline->GetStateObject());
		cmdList4->SetComputeRootSignature(m_rtPipeline->GetGlobalRootSignature());
	}

	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList4->SetDescriptorHeaps(1, heaps);

	// Param 0: TLAS
	D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV = descHeap.GetGPUHandle(m_tlas->GetSRVIndex());
	cmdList4->SetComputeRootDescriptorTable(0, tlasSRV);

	// Param 1: Output UAV
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV = descHeap.GetGPUHandle(m_raytracingOutput.GetUAVIndex(0));
	cmdList4->SetComputeRootDescriptorTable(1, outputUAV);

	// Param 2: Camera (TODO)
	// ...

	// Param 3: Table SRVs
	//		Materials           (SRV)
	//		Vertex Buffer       (SRV)
	//		Index Buffer        (SRV)
	//		GeoInfo Buffer      (SRV)
	//		GeoInstBase Buffer  (SRV)
	D3D12_GPU_DESCRIPTOR_HANDLE materialsSRV = descHeap.GetGPUHandle(m_materials.GetSRVIndex());
	cmdList4->SetComputeRootDescriptorTable(3, materialsSRV);

	// DispatchRays
	if (m_usePathTracing)
	{
		D3D12_DISPATCH_RAYS_DESC desc = {
			.RayGenerationShaderRecord = m_ptShaderTable->GetRayGenRecord(),
			.MissShaderTable = m_ptShaderTable->GetMissTable(),
			.HitGroupTable = m_ptShaderTable->GetHitGroupTable(),
			.Width = m_raytracingOutput.GetWidth(),
			.Height = m_raytracingOutput.GetHeight(),
			.Depth = 1
		};
		cmdList4->DispatchRays(&desc);
		return;
	}

	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = m_rtShaderTable->GetRayGenRecord(),
		.MissShaderTable = m_rtShaderTable->GetMissTable(),
		.HitGroupTable = m_rtShaderTable->GetHitGroupTable(),
		.Width = m_raytracingOutput.GetWidth(),
		.Height = m_raytracingOutput.GetHeight(),
		.Depth = 1
	};
	cmdList4->DispatchRays(&desc);
}