#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include <unordered_set>
#include <Scene.h>
#include <DxFrameResource.h>
#include <DxDeviceGlobal.h>
#include <DxRendererGlobal.h>
#include <DxDescriptorHeapGlobal.h>
#include <DxCommandQueueGlobal.h>
#include <InputGlobal.h>
#include <MeshComponent.h>
#include <DxSwapChain.h>
#include <DxBLAS.h>
#include <DxBuffer.h>
#include <DxUploadHeap.h>
#include <DxUtils.h>

DxrRenderPass::DxrRenderPass(uint32_t width, uint32_t height) : m_width(width), m_height(height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Constructor: {}x{}\n", width, height);
}

void DxrRenderPass::Initialize()
{
	DEBUG_LOG_FMT("[DxrRenderPass] Initializing with resolution: {}x{}\n", m_width, m_height);

	auto&				  device = GLOBAL(DxDeviceGlobal);
	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));
	m_tlas = std::make_unique<DxTLAS>();
	m_tlas->Initialize(device5.Get(), 2000);

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	DEBUG_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	m_meshCache.clear();
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

void DxrRenderPass::CreateGeometryDescriptorTable()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	if (m_geometryTableRange.count > 0)
	{
		auto& queue = GLOBAL(DxGfxCommandQueueGlobal);
		auto  fenceValue = queue.GetCurrentFenceValue();
		descHeap.FreeRange(
			m_geometryTableRange.startIndex, m_geometryTableRange.count, FenceHandle{EQueueType::Graphics, fenceValue},
			"DXR_GeometryTable_Old"
		);
	}

	DxDescriptorTableBuilder tableBuilder;

	// Material SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC materialSRV{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_materials.Size()),
			 .StructureByteStride = sizeof(PBRMaterial),
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_materials.GetBuffer()->GetResource(), &materialSRV);

	// Vertex SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC vertexSRV{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_vertices.Size()),
			 .StructureByteStride = sizeof(VertexPNU),
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_vertices.GetBuffer()->GetResource(), &vertexSRV);

	// Index SRV (Raw Buffer)
	D3D12_SHADER_RESOURCE_VIEW_DESC indexSRV{
		.Format = DXGI_FORMAT_R32_UINT,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_indices.Size()),
			 .StructureByteStride = 0,
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_indices.GetBuffer()->GetResource(), &indexSRV);

	// GeoInfo SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC geoInfoSRV{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_geoInfos.Size()),
			 .StructureByteStride = sizeof(GeoInfo),
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_geoInfos.GetBuffer()->GetResource(), &geoInfoSRV);

	// InstGeoBase SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC instGeoSRV{
		.Format = DXGI_FORMAT_R32_UINT,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_instGeoBase.Size()),
			 .StructureByteStride = 0,
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_instGeoBase.GetBuffer()->GetResource(), &instGeoSRV);

	m_geometryTableRange = tableBuilder.Commit(device.GetDevice(), descHeap);

	DEBUG_LOG_FMT(
		"[DxrRenderPass] Geometry descriptor table created: StartIndex={}, Count={}\n", m_geometryTableRange.startIndex,
		m_geometryTableRange.count
	);
}

void DxrRenderPass::CollectRenderData(Scene* scene)
{
	auto* meshStorage = scene->GetStorage<MeshComponent>();
	if (!meshStorage)
		return;

	m_vertices.Clear();
	m_indices.Clear();
	m_materials.Clear();
	m_geoInfos.Clear();
	m_instGeoBase.Clear();

	uint32_t currentVertexBase = 0;
	uint32_t currentIndexBase = 0;
	uint32_t currentGeoIndex = 0;

	// TODO: Material Management (MeshComponent?)
	PBRMaterial defaultMat;
	defaultMat.albedo = {0.8f, 0.8f, 0.8f};
	defaultMat.metallic = 0.0f;
	defaultMat.roughness = 0.5f;
	defaultMat.emissive = {0.0f, 0.0f, 0.0f};
	defaultMat.emissiveStrength = 0.0f;

	uint32_t validMeshCount = 0;
	for (const auto& mesh : meshStorage->GetList())
	{
		if (!mesh.IsValid())
		{
			continue;
		}

		validMeshCount++;

		const auto& vertices = mesh.GetVertices();
		const auto& indices = mesh.GetIndices();

		for (const auto& v : vertices)
		{
			m_vertices.Register(VertexPNU{v.position, v.normal, v.uv});
		}

		for (auto idx : indices)
		{
			m_indices.Register(idx);
		}

		m_materials.Register(defaultMat);

		m_geoInfos.Register(
			currentVertexBase, currentIndexBase, static_cast<uint32_t>(vertices.size()),
			static_cast<uint32_t>(indices.size())
		);

		m_instGeoBase.Register(currentGeoIndex);

		currentVertexBase += static_cast<uint32_t>(vertices.size());
		currentIndexBase += static_cast<uint32_t>(indices.size());
		currentGeoIndex++;
	}

	if (m_tlas && m_tlas->GetInstanceCount() != validMeshCount)
	{
		m_needsRebuild = true;
		DEBUG_LOG_FMT(
			"[DxrRenderPass] Instance count changed: {} -> {}, TLAS rebuild required\n", m_tlas->GetInstanceCount(),
			validMeshCount
		);
	}
}

void DxrRenderPass::BuildAccelerationStructures(Scene* scene)
{
	auto& renderer = GLOBAL(DxRendererGlobal);
	auto* frame = renderer.GetCurrentFrame();
	if (!frame)
	{
		return;
	}

	auto* context = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();
	auto* cmdList = context->CommandList();
	auto& device = GLOBAL(DxDeviceGlobal);

	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	if (!scene)
	{
		return;
	}

	std::vector<std::pair<GameObject*, DxBLAS*>> instances;
	std::unordered_set<HandleOf<MeshComponent>>	 currentFrameMeshes;

	uint64_t currentFrame = renderer.GetSwapChain()->GetFrameCount();
	auto*	 meshStorage = scene->GetStorage<MeshComponent>();
	if (!meshStorage)
	{
		return;
	}

	for (auto& mesh : meshStorage->GetList())
	{
		if (!mesh.IsValid())
		{
			continue;
		}

		auto meshHandle = mesh.GetHandle();
		// auto* mesh = mesh.GetMesh();
		// if (!mesh)
		// {
		// 	continue;
		// }
		// if (mesh->GetVertices().empty() || mesh->GetIndices().empty())
		// {
		// 	continue;
		// }
		// auto meshID = mesh->GetResourceID();

		currentFrameMeshes.insert(meshHandle);

		auto it = m_meshCache.find(meshHandle);
		if (it == m_meshCache.end())
		{
			MeshRenderResource res;
			res.vertexCount = mesh.GetVertices().size();
			res.indexCount = mesh.GetIndices().size();

			// Upload Vertex Buffer
			uint64_t vSize = res.vertexCount * sizeof(Vertex);
			res.vertexBuffer = std::make_unique<DxBuffer>();
			res.vertexBuffer->Initialize(
				device5.Get(), vSize, EBufferUsage::Vertex, D3D12_RESOURCE_FLAG_NONE,
				"BLAS_VB_" + mesh.GetGameObject()->GetName()
			);
			auto vUpload = uploadHeap->UploadVertexBuffer(mesh.GetVertices());

			D3D12_RESOURCE_BARRIER vBarriers[2];
			vBarriers[0] = DxUtils::CreateTransitionBarrier(
				res.vertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
			);
			cmdList->ResourceBarrier(1, &vBarriers[0]);
			cmdList->CopyBufferRegion(
				res.vertexBuffer->GetResource(), 0, uploadHeap->GetResource(), vUpload.offset, vSize
			);
			vBarriers[1] = DxUtils::CreateTransitionBarrier(
				res.vertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
			cmdList->ResourceBarrier(1, &vBarriers[1]);

			// Upload Index Buffer
			uint64_t iSize = res.indexCount * sizeof(uint32_t);
			res.indexBuffer = std::make_unique<DxBuffer>();
			res.indexBuffer->Initialize(
				device5.Get(), iSize, EBufferUsage::Index, D3D12_RESOURCE_FLAG_NONE,
				"BLAS_IB_" + mesh.GetGameObject()->GetName()
			);
			auto iUpload = uploadHeap->UploadIndexBuffer(mesh.GetIndices());

			D3D12_RESOURCE_BARRIER iBarriers[2];
			iBarriers[0] = DxUtils::CreateTransitionBarrier(
				res.indexBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
			);
			cmdList->ResourceBarrier(1, &iBarriers[0]);
			cmdList->CopyBufferRegion(
				res.indexBuffer->GetResource(), 0, uploadHeap->GetResource(), iUpload.offset, iSize
			);
			iBarriers[1] = DxUtils::CreateTransitionBarrier(
				res.indexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
			cmdList->ResourceBarrier(1, &iBarriers[1]);

			// Build BLAS
			res.blas = std::make_unique<DxBLAS>();
			res.blas->Build(
				device5.Get(), cmdList4.Get(), res.vertexBuffer->GetGPUAddress(),
				static_cast<uint32_t>(res.vertexCount), sizeof(Vertex), res.indexBuffer->GetGPUAddress(),
				static_cast<uint32_t>(res.indexCount), false, "BLAS_" + mesh.GetGameObject()->GetName()
			);

			m_meshCache[meshHandle] = std::move(res);
			it = m_meshCache.find(meshHandle);

			DEBUG_LOG_FMT(
				"[DxrRenderPass] Created BLAS for mesh {} (Handle: {})\n", mesh.GetGameObject()->GetName(),
				meshHandle.id
			);
		}

		it->second.lastUsedFrame = currentFrame;

		auto* obj = mesh.GetGameObject();
		if (obj && it->second.blas && it->second.blas->IsBuilt())
		{
			instances.push_back({obj, it->second.blas.get()});
		}
	}

	if (m_tlas && !instances.empty())
	{
		if (m_needsRebuild || !m_tlas->IsBuilt())
		{
			m_tlas->Build(device5.Get(), cmdList4.Get(), instances);
			m_needsRebuild = false;
		}
		else
		{
			m_tlas->Refit(device5.Get(), cmdList4.Get(), instances);
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

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
		DEBUG_LOG_FMT("[GameFramework] Switched to {}\n", m_usePathTracing ? "Path Tracing" : "Ray Tracing");
	}

	auto& device = GLOBAL(DxDeviceGlobal);
	auto* context = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();

	CollectRenderData(scene);

	m_vertices.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_indices.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_materials.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_geoInfos.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	m_instGeoBase.SyncToGPU(device.GetDevice(), *context, *uploadHeap);

	if (m_vertices.NeedsDescriptorUpdate() || m_indices.NeedsDescriptorUpdate() ||
		m_materials.NeedsDescriptorUpdate() || m_geoInfos.NeedsDescriptorUpdate() ||
		m_instGeoBase.NeedsDescriptorUpdate())
	{
		m_geometryTableValid = false;

		m_vertices.MarkDescriptorUpdated();
		m_indices.MarkDescriptorUpdated();
		m_materials.MarkDescriptorUpdated();
		m_geoInfos.MarkDescriptorUpdated();
		m_instGeoBase.MarkDescriptorUpdated();

		DEBUG_LOG_FMT("[DxrRenderPass] Buffer resized, geometry table invalidated\n");
	}

	BuildAccelerationStructures(scene);

	if (!m_tlas || !m_tlas->IsBuilt())
		return;

	if (!m_geometryTableValid || m_needsRebuild)
	{
		CreateGeometryDescriptorTable();
		m_geometryTableValid = true;
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

	D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV = descHeap.GetGPUHandle(m_tlas->GetSRVIndex());
	cmdList4->SetComputeRootDescriptorTable(0, tlasSRV);

	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV = descHeap.GetGPUHandle(m_raytracingOutput.GetUAVIndex(0));
	cmdList4->SetComputeRootDescriptorTable(1, outputUAV);

	// Camera
	auto mainCam = CameraComponent::GetMainViewMatrix();
	auto mainProj = CameraComponent::GetMainProjectionMatrix();
	auto viewProj = DX::XMMatrixMultiply(mainCam, mainProj);
	auto viewProjInv = DX::XMMatrixInverse(nullptr, viewProj);

	viewProjInv = DX::XMMatrixTranspose(viewProjInv);

	DX::XMFLOAT4X4 viewProjInvFloat;
	DX::XMStoreFloat4x4(&viewProjInvFloat, viewProjInv);

	cmdList4->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);

	// Tables
	cmdList4->SetComputeRootDescriptorTable(3, descHeap.GetGPUHandle(m_geometryTableRange.startIndex));

	// Dispatch
	auto*					 shaderTable = m_usePathTracing ? m_ptShaderTable.get() : m_rtShaderTable.get();
	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = shaderTable->GetRayGenRecord(),
		.MissShaderTable = shaderTable->GetMissTable(),
		.HitGroupTable = shaderTable->GetHitGroupTable(),
		.Width = m_raytracingOutput.GetWidth(),
		.Height = m_raytracingOutput.GetHeight(),
		.Depth = 1
	};
	cmdList4->DispatchRays(&desc);
}


#if 0
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

	m_vertices.Clear();
	m_indices.Clear();
	m_materials.Clear();
	m_geoInfos.Clear();
	m_instGeoBase.Clear();

	uint32_t currentVertexBase = 0;
	uint32_t currentIndexBase = 0;
	uint32_t currentGeoIndex = 0;

	PBRMaterial defaultMat;
	defaultMat.albedo = {0.8f, 0.8f, 0.8f};
	defaultMat.metallic = 0.0f;
	defaultMat.roughness = 0.5f;
	defaultMat.emissive = {0.0f, 0.0f, 0.0f};
	defaultMat.emissiveStrength = 0.0f;

	for (const auto& mesh : meshStorage->GetList())
	{
		if (!mesh.IsValid())
			continue;

		const auto& vertices = mesh.GetVertices();
		const auto& indices = mesh.GetIndices();

		for (const auto& v : vertices)
		{
			m_vertices.Register(v.position, v.normal, v.uv);
		}

		for (auto idx : indices)
		{
			m_indices.Register(idx);
		}

		m_materials.Register(defaultMat);

		m_geoInfos.Register(
			currentVertexBase, currentIndexBase, static_cast<uint32_t>(vertices.size()),
			static_cast<uint32_t>(indices.size())
		);

		m_instGeoBase.Register(currentGeoIndex);

		currentVertexBase += static_cast<uint32_t>(vertices.size());
		currentIndexBase += static_cast<uint32_t>(indices.size());
		currentGeoIndex++;
	}

	m_needsRebuild = true;
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

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
		DEBUG_LOG_FMT("[GameFramework] Switched to {}\n", m_usePathTracing ? "Path Tracing" : "Ray Tracing");
	}

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
#endif