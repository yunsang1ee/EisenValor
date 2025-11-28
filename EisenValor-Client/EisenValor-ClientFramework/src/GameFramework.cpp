#include "stdafxClientFramework.h"
#include "GameFramework.h"

#include "GlobalInterfaces.h"
#include "DxDeviceGlobal.h"
#include "DxDebugGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxShaderCompilerGlobal.h"
#include "InputGlobal.h"
#include "TimerGlobal.h"
#include "DxUtils.h"
#include "DxFeatureCaps.h"
#include "GameObjectManager.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxCommandContext.h"

#include "Actor.h"
#include "MeshComponent.h"
#include "DxBLAS.h"


#define SERVER

bool GameFramework::Initialize(HINSTANCE hInstance, HWND hwnd)
{
#ifdef SERVER
	NetBridge::ServerPacketHandler::Init();

	if (false == MANAGER(NetBridge::NetworkManager).Init())
		return false;
#endif
	m_hInstance = hInstance;
	m_hWnd = hwnd;

	Globals::Initialize();
	auto& time = MANAGER(TimerGlobal);
	time.SetFixedFPS(60);
	time.SetTargetFPS(0);

	auto& device = MANAGER(DxDeviceGlobal);
	m_featureCaps = DxFeatureCaps::Query(device.GetDevice(), device.GetAdapter());
	m_featureCaps.LogCapabilities();

	if (m_featureCaps.rayTracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
	{
		DEBUG_LOG_FMT("[GameFramework] ERROR: DXR not supported on this device!\n");
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = kFrameCount;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device.GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
	m_rtvDescriptorSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 윈도우 크기 가져오기
	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);
	uint32_t width = clientRect.right - clientRect.left;
	uint32_t height = clientRect.bottom - clientRect.top;

	// 스왑체인 생성
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	m_swapChain = std::make_unique<DxSwapChain>(
		device.GetDevice(), device.GetFactory(), commandQueue, m_hWnd, width, height, kFrameCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_rtvDescriptorSize
	);

	m_swapChain->SetResizeCallback(
		[this](uint32_t w, uint32_t h)
		{
			// RecreateDepthStencilBuffer(w, h);
			ResizeRaytracingResources(w, h);
		}
	);

	//-------------------------- 깊이 버퍼용 ------------
	//// DSV 디스크립터 힙 생성 (깊이 버퍼용)
	// D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	// dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	// dsvHeapDesc.NumDescriptors = 1;
	// dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// ThrowIfFailed(device.GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvDescriptorHeap)));
	// m_dsvDescriptorSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//
	//  깊이 버퍼 생성
	// RecreateDepthStencilBuffer(width, height);

	//---------------------------------------------------

	for (uint32_t i = 0; i < kFrameCount; ++i)
	{
		m_frameResources[i] = std::make_unique<DxFrameResource>();
		m_frameResources[i]->Initialize(device.GetDevice(), i);
	}

	CreateRaytracingResources(width, height);

	CreateStaticScene();
	BuildAccelerationStructures();
	CreateBuffers();
	CreateRaytracingPipeline();

	UpdateCameraVectors();

	std::string id, pw;
	std::cout << "Input ID(any):";
	// std::cin >> id;
	id = "ID";
	std::cout << "\n";
	std::cout << "Input PW(any):";
	// std::cin >> pw;
	pw = "PW";

	auto pb = NetBridge::ServerPacketHandler::Make_CS_LOGIN_PACKET(id.c_str(), pw.c_str());
	MANAGER(NetBridge::NetworkManager).Send(std::move(pb));

	return true;
}

// void GameFramework::RecreateDepthStencilBuffer(uint32_t width, uint32_t height)
//{
//	auto& device = MANAGER(DxDeviceGlobal);
//	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
//
//	commandQueue.WaitForIdle();
//	m_depthStencilBuffer.Reset();
//
//	D3D12_RESOURCE_DESC depthStencilDesc = {};
//	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	depthStencilDesc.Width = width;
//	depthStencilDesc.Height = height;
//	depthStencilDesc.DepthOrArraySize = 1;
//	depthStencilDesc.MipLevels = 1;
//	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24비트 깊이 + 8비트 스텐실
//	depthStencilDesc.SampleDesc.Count = 1;
//	depthStencilDesc.SampleDesc.Quality = 0;
//	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
//	depthOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
//	depthOptimizedClearValue.DepthStencil.Stencil = 0;
//
//	D3D12_HEAP_PROPERTIES depthHeapProps = {};
//	depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
//
//	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
//		&depthHeapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
//		&depthOptimizedClearValue, IID_PPV_ARGS(&m_depthStencilBuffer)
//	));
//
//	// 깊이 스텐실 뷰 생성
//	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
//	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
//	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
//
//	device.GetDevice()->CreateDepthStencilView(
//		m_depthStencilBuffer.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
//	);
//
//	DEBUG_LOG_FMT("[GameFramework] Depth stencil buffer recreated: {}x{}\n", width, height);
// }

void GameFramework::CreateStaticScene()
{
	auto& device = MANAGER(DxDeviceGlobal);

	auto ground = std::make_unique<Actor>("Ground");
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
	m_sceneActors.push_back(std::move(ground));

	PBRMaterial groundMaterial;
	groundMaterial.albedo = {0.9f, 0.9f, 0.9f};
	groundMaterial.metallic = 0.98f;
	groundMaterial.roughness = 0.01;
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
		auto player = std::make_unique<Actor>("Player" + std::to_string(i));
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

		m_sceneActors.push_back(std::move(player));
		m_materials.push_back(playerMaterial[i]);
	}

	DEBUG_LOG_FMT("[GameFramework] Created static scene: {} actors\n", m_sceneActors.size());
}

void GameFramework::BuildAccelerationStructures()
{
	auto& device = MANAGER(DxDeviceGlobal);
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);

	auto* frame = m_frameResources[0].get();
	frame->BeginFrame();

	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* uploadHeap = frame->GetUploadHeap();

	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	for (auto& actor : m_sceneActors)
	{
		auto* mesh = actor->GetComponent<MeshComponent>();
		if (mesh)
		{
			mesh->BuildBLAS(device5.Get(), cmdList4.Get(), uploadHeap);
		}
	}

	std::vector<Actor*> actorPtrs;
	actorPtrs.reserve(m_sceneActors.size());
	for (auto& actor : m_sceneActors)
	{
		actorPtrs.push_back(actor.get());
	}

	m_tlas = std::make_unique<DxTLAS>();
	m_tlas->Build(device5.Get(), cmdList4.Get(), uploadHeap, actorPtrs);

	frame->ExecuteAndSignal(commandQueue.GetQueue());
	frame->WaitForCompletion();

	DEBUG_LOG_FMT("[GameFramework] Acceleration structures built\n");
}

void GameFramework::CreateRaytracingPipeline()
{
	auto&				  device = MANAGER(DxDeviceGlobal);
	ComPtr<ID3D12Device5> device5;
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));

	m_rtPipeline = std::make_unique<DxRtPipelineState>();
	m_rtPipeline->Create(device5.Get(), L"Resource/Shader/RaytracingLibraryPT.hlsl", 19);

	m_shaderTable = std::make_unique<DxRtShaderTable>();
	m_shaderTable->Build(device5.Get(), m_rtPipeline.get(), 1);

	DEBUG_LOG_FMT("[GameFramework] Raytracing pipeline created\n");
}

void GameFramework::CreateBuffers()
{
	auto& device = MANAGER(DxDeviceGlobal);
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	if (m_sceneActors.empty())
	{
		DEBUG_LOG_FMT("[GameFramework] WARNING: No actors for geometry buffers\n");
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

	for (auto& actor : m_sceneActors)
	{
		auto* mesh = actor->GetComponent<MeshComponent>();
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
	m_bufferBatch = descHeap.ReserveBatch(5);

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
	m_materialBuffer.CreateSRVInBatch(device.GetDevice(), descHeap, m_bufferBatch.startIndex, &srvDesc1);

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
	m_vertexBuffer.CreateSRVInBatch(device.GetDevice(), descHeap, m_bufferBatch.startIndex + 1, &srvDesc2);

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
	m_indexBuffer.CreateSRVInBatch(device.GetDevice(), descHeap, m_bufferBatch.startIndex + 2, &srvDesc3);

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
	m_geoInfoBuffer.CreateSRVInBatch(device.GetDevice(), descHeap, m_bufferBatch.startIndex + 3, &srvDesc4);

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
	m_instGeoBaseBuffer.CreateSRVInBatch(device.GetDevice(), descHeap, m_bufferBatch.startIndex + 4, &srvDesc5);

	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	frame->ExecuteAndSignal(commandQueue.GetQueue());
	frame->WaitForCompletion();

	DEBUG_LOG_FMT("[GameFramework] Buffers created, SRV batch start: {}\n", m_bufferBatch.startIndex);
}

void GameFramework::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = MANAGER(DxDeviceGlobal);
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	m_raytracingOutput.Initialize(
		device.GetDevice(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "RaytracingOutput"
	);
	m_raytracingOutput.CreateUAV(device.GetDevice(), descHeap, 0);

	DEBUG_LOG_FMT(
		"[GameFramework] Raytracing output created: {}x{}, UAV Index={}\n", width, height,
		m_raytracingOutput.GetUAVIndex()
	);
}

void GameFramework::ResizeRaytracingResources(uint32_t width, uint32_t height)
{
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	auto fenceValue = m_frameResources[m_currentFrameIndex]->GetFenceValue();
	m_raytracingOutput.ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
	CreateRaytracingResources(width, height);
}


void GameFramework::Run()
{
#ifdef SERVER
	MANAGER(NetBridge::NetworkManager).ProcessIO();
#endif

	MANAGER(InputGlobal).BeforeUpdate();

	MANAGER(TimerGlobal).Update();
	if (MANAGER(TimerGlobal).ShouldFixedUpdate())
		FixedUpdate();
	Update();
	LateUpdate();

	Render();

	MANAGER(InputGlobal).AfterUpdate();
	MANAGER(GameObjectManager).FinalUpdate();
#ifdef _DEBUG
	MANAGER(DxDebugGlobal).PrintDebugMessages();
#endif
}

void GameFramework::Release()
{
	DEBUG_LOG_FMT("[GameFramework] Releasing resources...\n");

	auto& queue = MANAGER(DxGfxCommandQueueGlobal);
	auto& heap = MANAGER(DxDescriptorHeapGlobal);

	queue.WaitForIdle();

	if (m_raytracingOutput.HasUAV())
	{
		heap.FreeImmediate(m_raytracingOutput.GetUAVIndex());
		DEBUG_LOG_FMT("Released UAV: {}\n", m_raytracingOutput.GetUAVIndex());
	}
	if (m_bufferBatch.count > 0)
	{
		heap.FreeBatchImmediate(m_bufferBatch.startIndex, m_bufferBatch.count);
		DEBUG_LOG_FMT("  Released SRV batch: start={}, count={}\n", m_bufferBatch.startIndex, m_bufferBatch.count);
		m_bufferBatch = {0, 0};
	}

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
			MANAGER(InputGlobal).OnInputState(code, isPressed, isUp);
	}
	break;
	case WM_MOUSEMOVE:
		MANAGER(InputGlobal).OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
	{
		MANAGER(InputGlobal).OnInputState(VK_LBUTTON, FALSE, FALSE);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		MANAGER(InputGlobal).OnInputState(VK_RBUTTON, FALSE, FALSE);
		break;
	}
	case WM_LBUTTONUP:
	{
		MANAGER(InputGlobal).OnInputState(VK_LBUTTON, FALSE, TRUE);
		break;
	}
	case WM_RBUTTONUP:
	{
		MANAGER(InputGlobal).OnInputState(VK_RBUTTON, FALSE, TRUE);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		MANAGER(InputGlobal).OnWheelScroll(GET_WHEEL_DELTA_WPARAM(wParam));
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

void GameFramework::Update()
{
	auto& input = MANAGER(InputGlobal);
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
		m_swapChain->ToggleBorderlessFullscreen();
	}
	if (input.GetInput(VK_MENU) && input.GetInputDown(VK_RETURN))
	{
		m_swapChain->ToggleFullscreen();
	}

	static float runTime{};
	const float	 dt = MANAGER(TimerGlobal).GetDeltaTime();

	if ((runTime += dt) > 0.2f)
	{
		runTime = 0.0f;
		DEBUG_LOG_FMT("[GameFramework] FPS: {:.2f}, Frame Time: {:.2f}ms\n", 1.0f / dt, dt * 1000.0f);
	}

	UpdateCamera(dt);
	MANAGER(GameObjectManager).Update(dt);
}

void GameFramework::FixedUpdate() {}

void GameFramework::LateUpdate() {}

void GameFramework::Render()
{
	auto localPlayer = MANAGER(GameObjectManager).GetLocalPlayer();
	if (localPlayer == nullptr)
		return;

	m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();
	auto* frame = m_frameResources[m_currentFrameIndex].get();

	frame->BeginFrame();
	ComPtr<ID3D12GraphicsCommandList4> cmdList;
	ThrowIfFailed(frame->GetMainContext()->CommandList()->QueryInterface(IID_PPV_ARGS(&cmdList)));
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	RenderDXR();

	// RT Output → BackBuffer copy
	auto backBuffer = m_swapChain->GetCurrentBackBuffer();

	auto barrier1 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	auto barrier2 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_RESOURCE_BARRIER barriers[] = {barrier1, barrier2};
	cmdList->ResourceBarrier(2, barriers);

	cmdList->CopyResource(backBuffer, m_raytracingOutput.GetResource());

	// BackBuffer → Present, RT Output → UAV
	auto barrier3 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	auto barrier4 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	D3D12_RESOURCE_BARRIER restoreBarriers[] = {barrier3, barrier4};
	cmdList->ResourceBarrier(2, restoreBarriers);

	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	frame->ExecuteAndSignal(commandQueue.GetQueue());

	m_swapChain->PresentMaxPerformance();
}

void GameFramework::RenderDXR()
{
	auto*							   frame = m_frameResources[m_currentFrameIndex].get();
	ComPtr<ID3D12GraphicsCommandList4> cmdList;
	ThrowIfFailed(frame->GetMainContext()->CommandList()->QueryInterface(IID_PPV_ARGS(&cmdList)));
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	cmdList->SetPipelineState1(m_rtPipeline->GetStateObject());
	cmdList->SetComputeRootSignature(m_rtPipeline->GetGlobalRootSignature());

	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);

	// Param 0: TLAS (SRV)
	D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV = descHeap.GetGPUHandle(m_tlas->GetSRVIndex());
	cmdList->SetComputeRootDescriptorTable(0, tlasSRV);

	// Param 1: Output UAV
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV = descHeap.GetGPUHandle(m_raytracingOutput.GetUAVIndex());
	cmdList->SetComputeRootDescriptorTable(1, outputUAV);

	// Param 2: Camera Constants (임시 ViewProjInverse)
	DX::XMVECTOR cameraPos = DX::XMLoadFloat3(&m_cameraPosition);
	DX::XMVECTOR forwardVec = DX::XMLoadFloat3(&m_cameraForward);
	DX::XMVECTOR upVec = DX::XMLoadFloat3(&m_cameraUp);
	DX::XMVECTOR targetVec = DX::XMVectorAdd(cameraPos, forwardVec);

	DX::XMMATRIX view = DX::XMMatrixLookAtLH(cameraPos, targetVec, upVec);

	float aspectRatio = static_cast<float>(m_swapChain->GetWidth()) / static_cast<float>(m_swapChain->GetHeight());
	DX::XMMATRIX proj = DX::XMMatrixPerspectiveFovLH(DX::XM_PIDIV2, aspectRatio, 0.1f, 1000.0f);

	DX::XMMATRIX viewProj = view * proj;
	DX::XMMATRIX viewProjInverse = DX::XMMatrixInverse(nullptr, viewProj);

	viewProjInverse = DX::XMMatrixTranspose(viewProjInverse);
	DX::XMFLOAT4X4 viewProjInvFloat;
	DX::XMStoreFloat4x4(&viewProjInvFloat, viewProjInverse);
	cmdList->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);

	// Param 3: Table SRVs
	//		Materials           (SRV)
	//		Vertex Buffer       (SRV)
	//		Index Buffer        (SRV)
	//		GeoInfo Buffer      (SRV)
	//		GeoInstBase Buffer  (SRV)
	D3D12_GPU_DESCRIPTOR_HANDLE tableStart = descHeap.GetGPUHandle(m_materialBuffer.GetSRVIndex());
	cmdList->SetComputeRootDescriptorTable(3, tableStart);

	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = m_shaderTable->GetRayGenRecord(),
		.MissShaderTable = m_shaderTable->GetMissTable(),
		.HitGroupTable = m_shaderTable->GetHitGroupTable(),
		.Width = m_swapChain->GetWidth(),
		.Height = m_swapChain->GetHeight(),
		.Depth = 1
	};
	cmdList->DispatchRays(&desc);
}

void GameFramework::UpdateCamera(float deltaTime)
{
	auto& input = MANAGER(InputGlobal);

	// 우클릭으로 카메라 조작 활성화/비활성화
	if (input.GetInputDown(VK_RBUTTON))
	{
		m_cameraEnabled = true;
		m_firstMouse = true;
		ShowCursor(FALSE); // 마우스 커서 숨기기
	}
	else if (input.GetInputUp(VK_RBUTTON))
	{
		m_cameraEnabled = false;
		ShowCursor(TRUE); // 마우스 커서 보이기
	}

	if (!m_cameraEnabled)
		return;

	// 키보드 입력으로 이동
	float		 velocity = m_cameraSpeed * deltaTime;
	DX::XMVECTOR posVec = DX::XMLoadFloat3(&m_cameraPosition);
	DX::XMVECTOR forwardVec = DX::XMLoadFloat3(&m_cameraForward);
	DX::XMVECTOR rightVec = DX::XMLoadFloat3(&m_cameraRight);
	DX::XMVECTOR upVec = DX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

	// W/S: 전진/후진
	if (input.GetInput('W'))
		posVec = DX::XMVectorAdd(posVec, DX::XMVectorScale(forwardVec, velocity));
	if (input.GetInput('S'))
		posVec = DX::XMVectorSubtract(posVec, DX::XMVectorScale(forwardVec, velocity));

	// A/D: 좌/우 이동
	if (input.GetInput('A'))
		posVec = DX::XMVectorSubtract(posVec, DX::XMVectorScale(rightVec, velocity));
	if (input.GetInput('D'))
		posVec = DX::XMVectorAdd(posVec, DX::XMVectorScale(rightVec, velocity));

	// SPACE/SHIFT: 상/하 이동
	if (input.GetInput(VK_SPACE))
		posVec = DX::XMVectorAdd(posVec, DX::XMVectorScale(upVec, velocity));
	if (input.GetInput(VK_SHIFT))
		posVec = DX::XMVectorSubtract(posVec, DX::XMVectorScale(upVec, velocity));

	DX::XMStoreFloat3(&m_cameraPosition, posVec);

	// 마우스 이동으로 회전 (우클릭 중일 때만)
	auto mousePosition = input.GetMousePosition();

	if (m_firstMouse)
	{
		m_lastMouseX = mousePosition.x;
		m_lastMouseY = mousePosition.y;
		m_firstMouse = false;
	}

	float xOffset = static_cast<float>(mousePosition.x - m_lastMouseX);
	float yOffset = static_cast<float>(mousePosition.y - m_lastMouseY);

	m_lastMouseX = mousePosition.x;
	m_lastMouseY = mousePosition.y;

	xOffset *= m_mouseSensitivity;
	yOffset *= m_mouseSensitivity;

	m_cameraYaw += xOffset;
	m_cameraPitch -= yOffset;

	if (m_cameraPitch > 89.0f)
		m_cameraPitch = 89.0f;
	if (m_cameraPitch < -89.0f)
		m_cameraPitch = -89.0f;
	if (m_cameraYaw >= 360.f)
		m_cameraYaw -= 360.f;
	if (m_cameraYaw < 0.f)
		m_cameraYaw += 360.f;

	UpdateCameraVectors();
}

void GameFramework::UpdateCameraVectors()
{
	// Forward 벡터 계산
	float		 yawR = DX::XMConvertToRadians(m_cameraYaw);
	float		 pitchR = DX::XMConvertToRadians(m_cameraPitch);
	DX::XMFLOAT3 forward = {sinf(yawR) * cosf(pitchR), sinf(pitchR), cosf(yawR) * cosf(pitchR)};
	DX::XMVECTOR forwardVec = DX::XMLoadFloat3(&forward);
	forwardVec = DX::XMVector3Normalize(forwardVec);
	DX::XMStoreFloat3(&m_cameraForward, forwardVec);

	const DX::XMVECTOR worldUpVec = DX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

	// Right 벡터 계산
	DX::XMVECTOR rightVec = DX::XMVector3Normalize(DX::XMVector3Cross(worldUpVec, forwardVec));
	DX::XMStoreFloat3(&m_cameraRight, rightVec);

	// Up 벡터 계산
	DX::XMVECTOR upVec = DX::XMVector3Normalize(DX::XMVector3Cross(forwardVec, rightVec));
	DX::XMStoreFloat3(&m_cameraUp, upVec);
}
