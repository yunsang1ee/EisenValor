#include "stdafxClientFramework.h"
#include "GameFramework.h"

#include "GlobalInterfaces.h"
#include "DxDebugGlobal.h"
#include "SceneGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"
#include "InputGlobal.h"
#include "TimerGlobal.h"


#define SERVER

#if 0
bool GameFramework::Initialize(HINSTANCE hInstance, HWND hwnd)
{
#ifdef SERVER
	if (false == GLOBAL(NetBridge::NetworkGlobal).Init())
		return false;
#endif
	m_hInstance = hInstance;
	m_hWnd = hwnd;

	Globals::Initialize();
	auto& time = GLOBAL(TimerGlobal);
	time.SetFixedFPS(60);
	time.SetTargetFPS(0);

	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);
	uint32_t width = clientRect.right - clientRect.left;
	uint32_t height = clientRect.bottom - clientRect.top;

	CreateRaytracingResources(width, height);

	CreateStaticScene();
	BuildAccelerationStructures();
	CreateBuffers();
	CreateRaytracingPipeline();

	return true;
}

void GameFramework::CreateStaticScene()
{
	auto& device = GLOBAL(DxDeviceGlobal);

	auto ground = std::make_unique<GameObject>("Ground");
	ground->GetTransform().SetPosition(0.0f, 0.0f, 0.0f);
	ground->GetTransform().SetScale(10.0f);

	std::vector<Vertex> groundVertices = {
		{{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
		{{1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
		{{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
		{{-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}}
	};

	std::vector<uint32_t> groundIndices = {0, 2, 1, 0, 3, 2};

	auto groundMesh = ground->AddComponent<MeshComponent>();
	groundMesh->SetMesh(groundVertices, groundIndices);
	m_sceneObjects.push_back(std::move(ground));

	PBRMaterial groundMaterial;
	groundMaterial.albedo = {1.0f, 0.95f, 0.8f};
	groundMaterial.metallic = 0.08f;
	groundMaterial.roughness = 0.98f;
	groundMaterial.emissive = {0.0f, 0.0f, 0.0f};
	groundMaterial.emissiveStrength = 0.0f;
	m_materials.push_back(groundMaterial);

	DX::XMFLOAT3 rotations[3] = {{10.0f, 15.0f, 0.0f}, {-5.0f, 120.0f, 3.0f}, {8.0f, 210.0f, -10.0f}};
	PBRMaterial	 playerMaterial[3] = {
		 {.albedo = {0.9f, 0.3f, 0.3f},
		  .metallic = 0.98f,
		  .roughness = 0.01f,
		  .emissive = {1.0f, 0.4f, 0.2f},
		  .emissiveStrength = 0.0f},
		 {.albedo = {0.3f, 0.3f, 0.9f},
		  .metallic = 0.98f,
		  .roughness = 0.01f,
		  .emissive = {0.0f, 0.0f, 0.0f},
		  .emissiveStrength = 0.0f},
		 {.albedo = {0.3f, 0.9f, 0.3f},
		  .metallic = 0.98f,
		  .roughness = 0.01f,
		  .emissive = {0.2f, 0.4f, 1.0f},
		  .emissiveStrength = 0.0f},
	 };

	for (int i = 0; i < 3; ++i)
	{
		auto player = std::make_unique<GameObject>("Player" + std::to_string(i));
		player->GetTransform().SetPosition(4.0f * (i - 1), (i != 1 ? 3.0f : 1.0f), 0.0f);
		player->GetTransform().SetRotation(rotations[i]);
		player->GetTransform().SetScale((i != 1 ? 4.0f : 1.0f));

		// clang-format off
		std::vector<Vertex> cubeVertices = {
			// Front face (z = 0.5)
			{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
			{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

			// Back face (z = -0.5)
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
			{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
			{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

			// Left face (x = -0.5)
			{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
			{{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			{{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
			{{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},

			// Right face (x = 0.5)
			{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
			{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
			{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},

			// Top face (y = 0.5)
			{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
			{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
			{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},

			// Bottom face (y = -0.5)
			{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
			{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}
		};

		std::vector<uint32_t> cubeIndices = {
			0, 1, 2, 0, 2, 3,      // Front
			4, 5, 6, 4, 6, 7,      // Back
			8, 9, 10, 8, 10, 11,   // Left
			12, 13, 14, 12, 14, 15, // Right
			16, 17, 18, 16, 18, 19, // Top
			20, 21, 22, 20, 22, 23  // Bottom
		};
		// clang-format on

		auto playerMesh = player->AddComponent<MeshComponent>();
		playerMesh->SetMesh(cubeVertices, cubeIndices);

		m_sceneObjects.push_back(std::move(player));
		m_materials.push_back(playerMaterial[i]);
	}

	DEBUG_LOG_FMT("[GameFramework] Created static scene: {} objects\n", m_sceneObjects.size());
}

void GameFramework::BuildAccelerationStructures()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);

	auto* frame = m_frameResources[0].get();
	frame->BeginFrame();

	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* uploadHeap = frame->GetUploadHeap();

	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	for (auto& obj : m_sceneObjects)
	{
		auto* mesh = obj->GetComponent<MeshComponent>();
		if (mesh)
		{
			mesh->BuildBLAS(device5.Get(), cmdList4.Get(), uploadHeap);
		}
	}

	std::vector<GameObject*> objPtrs;
	objPtrs.reserve(m_sceneObjects.size());
	for (auto& obj : m_sceneObjects)
	{
		objPtrs.push_back(obj.get());
	}

	m_tlas = std::make_unique<DxTLAS>();
	m_tlas->Build(device5.Get(), cmdList4.Get(), uploadHeap, objPtrs);

	frame->ExecuteAndSignal(commandQueue.GetQueue());
	frame->WaitForCompletion();

	DEBUG_LOG_FMT("[GameFramework] Acceleration structures built\n");
}

void GameFramework::CreateRaytracingPipeline()
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

void GameFramework::CreateBuffers()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	if (m_sceneObjects.empty())
	{
		DEBUG_LOG_FMT("[GameFramework] WARNING: No objects for geometry buffers\n");
		return;
	}
	if (m_materials.empty())
	{
		DEBUG_LOG_FMT("[GameFramework] WARNING: No materials to upload\n");
	}

	// 1) CPU 데이터 수집
	m_allVertices.clear();
	m_allIndices.clear();
	m_geoInfoTable.clear();
	m_instGeoBase.clear();

	for (auto& obj : m_sceneObjects)
	{
		auto* mesh = obj->GetComponent<MeshComponent>();
		if (!mesh)
		{
			continue;
		}
		uint32_t geoBase = static_cast<uint32_t>(m_geoInfoTable.size());

		// IDEA: MeshComponent가 여러 개의 서브메시를 가질 수 있도록 확장 가능
		// for (auto& subMesh : mesh->GetSubMeshes())
		{
			uint32_t vertexBase = static_cast<uint32_t>(m_allVertices.size());
			uint32_t indexBase = static_cast<uint32_t>(m_allIndices.size());

			auto& vertices = mesh->GetVertices();
			auto& indices = mesh->GetIndices();

			for (auto& v : vertices)
			{
				m_allVertices.emplace_back(v.position, v.normal, DX::XMFLOAT2{0.0f, 0.0f});
			}

			m_allIndices.insert(m_allIndices.end(), indices.begin(), indices.end());

			m_geoInfoTable.emplace_back(
				vertexBase, indexBase, static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(indices.size())
			);
		}

		m_instGeoBase.push_back(geoBase);
	}

	DEBUG_LOG_FMT(
		"[GameFramework] Collected geometry: {} vertices, {} indices, {} geos\n", m_allVertices.size(),
		m_allIndices.size(), m_geoInfoTable.size()
	);

	// 2) GPU 버퍼 생성 + 업로드
	auto* frame = m_frameResources[0].get();
	frame->BeginFrame();
	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* uploadHeap = frame->GetUploadHeap();

	// --- Material Buffer (t1) ---
	{
		uint64_t bufSize = m_materials.size() * sizeof(PBRMaterial);
		m_materialBuffer.Initialize(
			device.GetDevice(), bufSize, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, "MaterialBuffer"
		);

		auto barrier1 = DxUtils::CreateTransitionBarrier(
			m_materialBuffer.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrier1);

		auto upload = uploadHeap->UploadRawData(m_materials.data(), bufSize, 256);
		cmdList->CopyBufferRegion(m_materialBuffer.GetResource(), 0, uploadHeap->GetResource(), upload.offset, bufSize);

		auto barrier2 = DxUtils::CreateTransitionBarrier(
			m_materialBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier2);
	}

	// --- Vertex Buffer (t2) ---
	{
		uint64_t bufSize = m_allVertices.size() * sizeof(VertexPNU);
		m_vertexBuffer.Initialize(
			device.GetDevice(), bufSize, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, "VertexBuffer_PNU"
		);

		auto barrier1 = DxUtils::CreateTransitionBarrier(
			m_vertexBuffer.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrier1);

		auto upload = uploadHeap->UploadRawData(m_allVertices.data(), bufSize, 256);
		cmdList->CopyBufferRegion(m_vertexBuffer.GetResource(), 0, uploadHeap->GetResource(), upload.offset, bufSize);

		auto barrier2 = DxUtils::CreateTransitionBarrier(
			m_vertexBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier2);
	}

	// --- Index Buffer (t3) ---
	{
		uint64_t bufSize = m_allIndices.size() * sizeof(uint32_t);
		m_indexBuffer.Initialize(
			device.GetDevice(), bufSize, EBufferUsage::Index, D3D12_RESOURCE_FLAG_NONE, "IndexBuffer"
		);

		auto barrier1 = DxUtils::CreateTransitionBarrier(
			m_indexBuffer.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrier1);

		auto upload = uploadHeap->UploadRawData(m_allIndices.data(), bufSize, 256);
		cmdList->CopyBufferRegion(m_indexBuffer.GetResource(), 0, uploadHeap->GetResource(), upload.offset, bufSize);

		auto barrier2 = DxUtils::CreateTransitionBarrier(
			m_indexBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier2);
	}

	// --- GeoInfo Buffer (t4) ---
	{
		uint64_t bufSize = m_geoInfoTable.size() * sizeof(GeoInfo);
		m_geoInfoBuffer.Initialize(
			device.GetDevice(), bufSize, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, "GeoInfoBuffer"
		);

		auto barrier1 = DxUtils::CreateTransitionBarrier(
			m_geoInfoBuffer.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrier1);

		auto upload = uploadHeap->UploadRawData(m_geoInfoTable.data(), bufSize, 256);
		cmdList->CopyBufferRegion(m_geoInfoBuffer.GetResource(), 0, uploadHeap->GetResource(), upload.offset, bufSize);

		auto barrier2 = DxUtils::CreateTransitionBarrier(
			m_geoInfoBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier2);
	}

	// --- InstGeoBase Buffer (t5) ---
	{
		uint64_t bufSize = m_instGeoBase.size() * sizeof(uint32_t);
		m_instGeoBaseBuffer.Initialize(
			device.GetDevice(), bufSize, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, "InstGeoBaseBuffer"
		);

		auto barrier1 = DxUtils::CreateTransitionBarrier(
			m_instGeoBaseBuffer.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrier1);

		auto upload = uploadHeap->UploadRawData(m_instGeoBase.data(), bufSize, 256);
		cmdList->CopyBufferRegion(
			m_instGeoBaseBuffer.GetResource(), 0, uploadHeap->GetResource(), upload.offset, bufSize
		);

		auto barrier2 = DxUtils::CreateTransitionBarrier(
			m_instGeoBaseBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier2);
	}

	// 3) 연속 슬롯에 SRV 생성 (t1~t5)
	DxDescriptorTableBuilder tableBuilder;
	// t1: Materials
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_materials.size()),
			 .StructureByteStride = sizeof(PBRMaterial),
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_materialBuffer.GetResource(), &srvDesc1, m_materialBuffer.GetSRVHandle());

	// t2: VertexBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_allVertices.size()),
			 .StructureByteStride = sizeof(VertexPNU),
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_vertexBuffer.GetResource(), &srvDesc2, m_vertexBuffer.GetSRVHandle());

	// t3: IndexBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3{
		.Format = DXGI_FORMAT_R32_UINT,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_allIndices.size()),
			 .StructureByteStride = 0,
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_indexBuffer.GetResource(), &srvDesc3, m_indexBuffer.GetSRVHandle());

	// t4: GeoInfoBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc4{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_geoInfoTable.size()),
			 .StructureByteStride = sizeof(GeoInfo),
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_geoInfoBuffer.GetResource(), &srvDesc4, m_geoInfoBuffer.GetSRVHandle());

	// t5: InstGeoBaseBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc5{
		.Format = DXGI_FORMAT_R32_UINT,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = static_cast<UINT>(m_instGeoBase.size()),
			 .StructureByteStride = 0,
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};
	tableBuilder.AddSRV(m_instGeoBaseBuffer.GetResource(), &srvDesc5, m_instGeoBaseBuffer.GetSRVHandle());

	m_bufferRange = tableBuilder.Commit(device.GetDevice(), descHeap);

	auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
	frame->ExecuteAndSignal(commandQueue.GetQueue());
	frame->WaitForCompletion();

	DEBUG_LOG_FMT("[GameFramework] Buffers created, SRV range start: {}\n", m_bufferRange.startIndex);
}

void GameFramework::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	m_raytracingOutput.Initialize(
		device.GetDevice(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "RaytracingOutput"
	);
	m_raytracingOutput.CreateUAV(device.GetDevice(), descHeap, 0);

	DEBUG_LOG_FMT(
		"[GameFramework] Raytracing output created: {}x{}, UAV Index={}\n", width, height,
		m_raytracingOutput.GetUAVIndex(0)
	);
}

void GameFramework::ResizeRaytracingResources(uint32_t width, uint32_t height)
{
	auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	auto fenceValue = commandQueue.GetCurrentFenceValue();
	m_raytracingOutput.ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
	CreateRaytracingResources(width, height);
}

void GameFramework::Run()
{
#ifdef SERVER
	GLOBAL(NetBridge::NetworkGlobal).ProcessIO();
#endif

	GLOBAL(InputGlobal).BeforeUpdate();

	static float runTime{};
	const float	 dt = GLOBAL(TimerGlobal).Update();

	if ((runTime += dt) > 0.2f)
	{
		runTime = 0.0f;
		DEBUG_LOG_FMT("[GameFramework] FPS: {:.2f}, Frame Time: {:.2f}ms\n", 1.0f / dt, dt * 1000.0f);
	}
	GLOBAL(SceneGlobal).OnBeginFrame();

	if (GLOBAL(TimerGlobal).ShouldFixedUpdate())
		FixedUpdate();

	Update(dt);


	Render();

	GLOBAL(SceneGlobal).OnEndFrame();

	GLOBAL(InputGlobal).AfterUpdate();
#ifdef _DEBUG
	GLOBAL(DxDebugGlobal).PrintDebugMessages();
#endif
}

void GameFramework::Release()
{
	DEBUG_LOG_FMT("[GameFramework] Releasing resources...\n");

	auto& queue = GLOBAL(DxGfxCommandQueueGlobal);
	queue.WaitForIdle();
	DEBUG_LOG_FMT("[GameFramework] Resource release complete\n");
}

LRESULT GameFramework::OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	WORD	  keyflags;
	bool	  isPressed;
	bool	  isUp;
	InputCode code;

	switch (message)
	{
	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU)
			return 0;
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		keyflags = HIWORD(lParam);
		isPressed = (keyflags & KF_REPEAT) == KF_REPEAT;
		isUp = (keyflags & KF_UP) == KF_UP;
		code = static_cast<InputCode>(wParam);
		if (code)
			GLOBAL(InputGlobal).OnInputState(code, isPressed, isUp);
	}
	break;
	case WM_MOUSEMOVE:
		GLOBAL(InputGlobal).OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
	{
		GLOBAL(InputGlobal).OnInputState(VK_LBUTTON, FALSE, FALSE);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		GLOBAL(InputGlobal).OnInputState(VK_RBUTTON, FALSE, FALSE);
		break;
	}
	case WM_LBUTTONUP:
	{
		GLOBAL(InputGlobal).OnInputState(VK_LBUTTON, FALSE, TRUE);
		break;
	}
	case WM_RBUTTONUP:
	{
		GLOBAL(InputGlobal).OnInputState(VK_RBUTTON, FALSE, TRUE);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		GLOBAL(InputGlobal).OnWheelScroll(GET_WHEEL_DELTA_WPARAM(wParam));
		break;
	}
	case WM_DESTROY:
	{
		DEBUG_LOG_FMT("Window destroyed. Initiating application shutdown.\n");
		PostQuitMessage(0);
		break;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void GameFramework::Update(float delta)
{
	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_ESCAPE))
	{
		DEBUG_LOG_FMT("close\n");
		::DestroyWindow(m_hWnd);
	}
	if (input.GetInputDown(VK_F5))
	{
		// FUTURE: Runtime Shader Compilation
	}
	if (input.GetInputDown(VK_F11))
	{
		GLOBAL(DxRendererGlobal).ToggleBorderlessFullscreen();
	}
	if (input.GetInput(VK_MENU) && input.GetInputDown(VK_RETURN))
	{
		GLOBAL(DxRendererGlobal).ToggleFullscreen();
	}

	GLOBAL(SceneGlobal).OnUpdate(delta);
}

void GameFramework::FixedUpdate()
{
	GLOBAL(SceneGlobal).OnFixedUpdate(GLOBAL(TimerGlobal).GetFixedDeltaTime());
}

void GameFramework::LateUpdate(float delta)
{
	GLOBAL(SceneGlobal).OnLateUpdate(delta);
}

void GameFramework::Render()
{
	auto& scene = GLOBAL(SceneGlobal);
	auto& renderer = GLOBAL(DxRendererGlobal);
	renderer.BeginFrame();

	renderer.Render(scene.GetActiveScene());

	renderer.EndFrame();


	// RenderDXR();
	// RT Output -> BackBuffer copy
	auto backBuffer = m_swapChain->GetCurrentBackBuffer();

	auto barrier1 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	auto barrier2 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_RESOURCE_BARRIER barriers[] = {barrier1, barrier2};
	cmdList->ResourceBarrier(2, barriers);

	cmdList->CopyResource(backBuffer, m_raytracingOutput.GetResource());

	// BackBuffer -> Present, RT Output -> UAV
	auto barrier3 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	auto barrier4 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	D3D12_RESOURCE_BARRIER restoreBarriers[] = {barrier3, barrier4};
	cmdList->ResourceBarrier(2, restoreBarriers);

	auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
	frame->ExecuteAndSignal(commandQueue.GetQueue());

	commandQueue.SignalFence();

	auto& gc = GLOBAL(DxGarbageCollectorGlobal);
	gc.ProcessCompletedReleases(commandQueue.GetCompletedFenceValue());

	m_swapChain->PresentMaxPerformance();
}
#endif

bool GameFramework::Initialize(HINSTANCE hInstance, HWND hwnd)
{
#ifdef SERVER
	if (false == GLOBAL(NetBridge::NetworkGlobal).Init())
		return false;
#endif

	m_hInstance = hInstance;
	m_hWnd = hwnd;

	Globals::Initialize(hwnd);

	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);
	uint32_t width = clientRect.right - clientRect.left;
	uint32_t height = clientRect.bottom - clientRect.top;

	auto& renderer = GLOBAL(DxRendererGlobal);
	renderer.CreateSwapChain(m_hWnd, width, height);

	DEBUG_LOG_FMT("[GameFramework] Initialized: {}x{}\n", width, height);
	return true;
}

void GameFramework::Run()
{
#ifdef SERVER
	GLOBAL(NetBridge::NetworkGlobal).ProcessIO();
#endif

	GLOBAL(InputGlobal).BeforeUpdate();

	static float runTime{};
	const float	 dt = GLOBAL(TimerGlobal).Update();

	//if ((runTime += dt) > 0.2f)
	//{
	//	runTime = 0.0f;
	//	DEBUG_LOG_FMT("[GameFramework] FPS: {:.2f}, Frame Time: {:.2f}ms\n", 1.0f / dt, dt * 1000.0f);
	//}

	GLOBAL(SceneGlobal).OnBeginFrame();

	if (GLOBAL(TimerGlobal).ShouldFixedUpdate())
		FixedUpdate();

	Update(dt);
	
	LateUpdate(dt);

	Render();

	GLOBAL(SceneGlobal).OnEndFrame();
	GLOBAL(InputGlobal).AfterUpdate();

#ifdef _DEBUG
	GLOBAL(DxDebugGlobal).PrintDebugMessages();
#endif
}

void GameFramework::Release()
{
	if (m_released)
		return;
	m_released = true;

	auto* queue = GLOBAL(DxGfxCommandQueueGlobal).GetQueue();
	if (!queue)
		return;

	DEBUG_LOG_FMT("[GameFramework] Releasing resources...\n");
	GLOBAL(DxGfxCommandQueueGlobal).WaitForIdle();
	DEBUG_LOG_FMT("[GameFramework] Resource release complete\n");

	Globals::Shutdown();
}

LRESULT GameFramework::OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	WORD	  keyflags;
	bool	  isPressed;
	bool	  isUp;
	InputCode code;

	switch (message)
	{
	case WM_ACTIVATE:
		GLOBAL(InputGlobal).SetWindowFocused(WA_ACTIVE == LOWORD(wParam));
		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU)
			return 0;
		break;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
			break;

		const uint32_t width = static_cast<uint32_t>(LOWORD(lParam));
		const uint32_t height = static_cast<uint32_t>(HIWORD(lParam));

		auto& renderer = GLOBAL(DxRendererGlobal);
		if (renderer.GetSwapChain())
		{
			renderer.OnResize(width, height);
		}
		GLOBAL(InputGlobal).OnResize(width, height);

		DEBUG_LOG_FMT("[GameFramework] Window resized: {}x{}\n", width, height);
		break;
	}

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		keyflags = HIWORD(lParam);
		isPressed = (keyflags & KF_REPEAT) == KF_REPEAT;
		isUp = (keyflags & KF_UP) == KF_UP;
		code = static_cast<InputCode>(wParam);
		if (code)
			GLOBAL(InputGlobal).OnInputState(code, isPressed, isUp);
	}
	break;

	case WM_MOUSEMOVE:
		GLOBAL(InputGlobal).OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONDOWN:
		GLOBAL(InputGlobal).OnInputState(VK_LBUTTON, FALSE, FALSE);
		break;

	case WM_RBUTTONDOWN:
		GLOBAL(InputGlobal).OnInputState(VK_RBUTTON, FALSE, FALSE);
		break;

	case WM_LBUTTONUP:
		GLOBAL(InputGlobal).OnInputState(VK_LBUTTON, FALSE, TRUE);
		break;

	case WM_RBUTTONUP:
		GLOBAL(InputGlobal).OnInputState(VK_RBUTTON, FALSE, TRUE);
		break;

	case WM_MOUSEWHEEL:
		GLOBAL(InputGlobal).OnWheelScroll(GET_WHEEL_DELTA_WPARAM(wParam));
		break;

	case WM_DESTROY:
		DEBUG_LOG_FMT("Window destroyed. Initiating application shutdown.\n");
		PostQuitMessage(0);
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void GameFramework::Update(float delta)
{
	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_ESCAPE))
	{
		DEBUG_LOG_FMT("close\n");
		::DestroyWindow(m_hWnd);
	}
	if (input.GetInputDown(VK_F5))
	{
		// FUTURE: Runtime Shader Compilation
	}

	if (auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain()) [[likely]]
	{
		if (input.GetInputDown(VK_F11))
		{
			swapChain->ToggleBorderlessFullscreen();
		}
		if (input.GetInput(VK_MENU) && input.GetInputDown(VK_RETURN))
		{
			swapChain->ToggleFullscreen();
		}
	}

	GLOBAL(SceneGlobal).OnUpdate(delta);
}

void GameFramework::FixedUpdate()
{
	GLOBAL(SceneGlobal).OnFixedUpdate(GLOBAL(TimerGlobal).GetFixedDeltaTime());
}

void GameFramework::LateUpdate(float delta)
{
	GLOBAL(SceneGlobal).OnLateUpdate(delta);
}

void GameFramework::Render()
{
	auto& scene = GLOBAL(SceneGlobal);
	auto& renderer = GLOBAL(DxRendererGlobal);

	renderer.BeginFrame();
	renderer.Render(scene.GetActiveScene());
	renderer.EndFrame();
}
