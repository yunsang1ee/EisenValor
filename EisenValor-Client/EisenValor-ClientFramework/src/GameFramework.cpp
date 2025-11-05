#include "stdafxClientFramework.h"
#include "GameFramework.h"

#include "GlobalInterfaces.h"
#include "DxDeviceGlobal.h"
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


using namespace DirectX;
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
	time.SetTargetFPS(144);

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
	CreateMaterialBuffer();
	CreateRaytracingPipeline();

	UpdateCameraVectors();

	// Legacy 파이프라인용
	/*
	// 루트 파라미터 정의 (상수 버퍼용)
	D3D_ROOT_SIGNATURE_VERSION targetVersion = std::min(
		m_featureCaps.rootSignature.HighestVersion,
		D3D_ROOT_SIGNATURE_VERSION_1_1 // 1.1로 제한
	);
	// 3. 루트 시그니처 생성 25.07.20 (수정 25.09.17)
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
	versionedRootSignatureDesc.Version = targetVersion;
	if (targetVersion == D3D_ROOT_SIGNATURE_VERSION_1_1)
	{ // Root Signature 1.1 사용
		D3D12_ROOT_PARAMETER1 rootParameter = {};
		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 상수 버퍼 뷰
		rootParameter.Descriptor.ShaderRegister = 0;				 // register(b0)
		rootParameter.Descriptor.RegisterSpace = 0;
		rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 정점 셰이더에서만 사용

		versionedRootSignatureDesc.Desc_1_1.NumParameters = 1;
		versionedRootSignatureDesc.Desc_1_1.pParameters = &rootParameter;
		versionedRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
		versionedRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
		versionedRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}
	else
	{ // Root Signature 1.0 사용
		D3D12_ROOT_PARAMETER rootParameter = {};
		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 상수 버퍼 뷰
		rootParameter.Descriptor.ShaderRegister = 0;				 // register(b0)
		rootParameter.Descriptor.RegisterSpace = 0;
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 정점 셰이더에서만 사용

		versionedRootSignatureDesc.Desc_1_0.NumParameters = 1;
		versionedRootSignatureDesc.Desc_1_0.pParameters = &rootParameter;
		versionedRootSignatureDesc.Desc_1_0.NumStaticSamplers = 0;
		versionedRootSignatureDesc.Desc_1_0.pStaticSamplers = nullptr;
		versionedRootSignatureDesc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&versionedRootSignatureDesc, &signature, &error));
	ThrowIfFailed(device.GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)
	));
	DEBUG_LOG_FMT(
		"[GameFramework] Using Root Signature Version: 1.{} (optimal choice)\n", static_cast<int>(targetVersion)
	);

	// 4. 셰이더 컴파일 (Simple)
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;


#ifdef _DEBUG
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	// 셰이더 파일에서 컴파일
	ThrowIfFailed(D3DCompileFromFile(
		L"../EisenValor/Resource/Shader/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0,
		&vertexShader, nullptr
	));
	ThrowIfFailed(D3DCompileFromFile(
		L"../EisenValor/Resource/Shader/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0,
		&pixelShader, nullptr
	));

	// 5. 입력 레이아웃 정의
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// PS 생성하기
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = {reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize()};
	psoDesc.PS = {reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize()};
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.DepthStencilState.DepthEnable = TRUE;

	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 깊이 정보를 쓸 것인가
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;	   // 깊이 비교

	psoDesc.DepthStencilState.StencilEnable = FALSE;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // 깊이 버퍼 포맷 설정

	ThrowIfFailed(device.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	*/

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

	// Ground 객체 생성 및 초기화
	// m_ground = std::make_unique<Ground>();
	// m_ground->Initialize(device.GetDevice());

	/*
	//Player 객체 생성 및 초기화 추가
	auto player = std::make_unique<Player>();
	player->SetPosition(0.0f, 0.5f, 0.0f);  // 초기 위치 설정
	player->Initialize(device.GetDevice());
	m_player = player.get();

	Objects들 추가
	m_gameObjects.push_back(std::move(player));

	적군 장수 생성
	auto enemyGeneral = std::make_shared<NPC>();
	enemyGeneral->SetTeam(NPC::Team::ENEMY);
	enemyGeneral->SetUnitType(NPC::UnitType::GENERAL);
	enemyGeneral->Initialize(device.GetDevice());
	Vec3 generalPos(0.0f, 0.0f, 8.0f);
	enemyGeneral->SetPosition(generalPos);
	enemyGeneral->lastServerPosition = generalPos;
	MANAGER(GameObjectManager).AddObject(enemyGeneral);

	적군 병사 생성 (장수 바로 옆에)
	auto enemySoldier = std::make_shared<NPC>();
	enemySoldier->SetTeam(NPC::Team::ENEMY);
	enemySoldier->SetUnitType(NPC::UnitType::SOLDIER);
	enemySoldier->Initialize(device.GetDevice());
	Vec3 soldierPos(1.5f, 0.0f, 8.0f);
	enemySoldier->SetPosition(soldierPos);
	enemySoldier->lastServerPosition = soldierPos;
	MANAGER(GameObjectManager).AddObject(enemySoldier);

	아군 배틀램 생성
	auto battleram = std::make_shared<NPC>();
	battleram->SetTeam(NPC::Team::ALLY);
	battleram->SetUnitType(NPC::UnitType::BATTLE_RAM);
	battleram->Initialize(device.GetDevice());
	Vec3 battleramPos(-3.0f, 0.0f, 2.0f);
	battleram->SetPosition(battleramPos);
	battleram->lastServerPosition = battleramPos;
	MANAGER(GameObjectManager).AddObject(battleram);

	아군 스폰 기지 생성
	auto allyspawnbase = std::make_shared<NPC>();
	allyspawnbase->SetTeam(NPC::Team::ALLY);
	allyspawnbase->SetUnitType(NPC::UnitType::SPAWN_BASE);
	allyspawnbase->Initialize(device.GetDevice());
	Vec3 spawnbasePos(-8.0f, 0.0f, 0.0f);
	allyspawnbase->SetPosition(spawnbasePos);
	allyspawnbase->lastServerPosition = spawnbasePos;
	MANAGER(GameObjectManager).AddObject(allyspawnbase);
	*/

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
	ground->SetPosition(0.0f, 0.0f, 0.0f);
	ground->SetScale(10.0f, 1.0f, 10.0f);

	std::vector<Vertex> groundVertices = {
		{{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
		{{1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
		{{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
		{{-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}}
	};

	std::vector<uint32_t> groundIndices = {0, 1, 2, 0, 2, 3};

	auto groundMesh = ground->AddComponent<MeshComponent>();
	groundMesh->SetMesh(groundVertices, groundIndices);
	m_sceneActors.push_back(std::move(ground));

	PBRMaterial groundMaterial;
	groundMaterial.albedo = {0.6f, 0.6f, 0.6f};
	groundMaterial.metallic = 0.0f;
	groundMaterial.roughness = 0.8f;
	groundMaterial.emissive = {0.0f, 0.0f, 0.0f};
	groundMaterial.emissiveStrength = 0.0f;
	m_materials.push_back(groundMaterial);

	for (int i = 0; i < 3; ++i)
	{
		auto player = std::make_unique<Actor>("Player" + std::to_string(i));
		player->SetPosition(-2.0f + i * 2.0f, 1.0f, 0.0f);
		player->SetScale(1.0f, 1.0f, 1.0f);

		// clang-format off
		std::vector<Vertex> cubeVertices = {
			{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},  // 0
			{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // 1
			{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},  // 2
			{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},  // 3
	
			{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},  // 4
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},  // 5
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},  // 6
			{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}}   // 7
		};

		std::vector<uint32_t> cubeIndices = {
			0, 1, 2, 0, 2, 3,  // Front
			5, 4, 7, 5, 7, 6,  // Back
			4, 0, 3, 4, 3, 7,  // Left
			1, 5, 6, 1, 6, 2,  // Right
			3, 2, 6, 3, 6, 7,  // Top
			4, 5, 1, 4, 1, 0   // Bottom
		};
		// clang-format on

		auto playerMesh = player->AddComponent<MeshComponent>();
		playerMesh->SetMesh(cubeVertices, cubeIndices);

		m_sceneActors.push_back(std::move(player));
		PBRMaterial playerMaterial;
		if (i == 0)
		{
			// 첫 번째: 메탈릭 재질
			playerMaterial.albedo = {0.9f, 0.3f, 0.3f};
			playerMaterial.metallic = 0.9f;
			playerMaterial.roughness = 0.1f;
			playerMaterial.emissive = {0.0f, 0.0f, 0.0f};
			playerMaterial.emissiveStrength = 0.0f;
		}
		else if (i == 1)
		{
			// 두 번째: 발광 재질
			playerMaterial.albedo = {0.3f, 0.3f, 0.9f};
			playerMaterial.metallic = 0.2f;
			playerMaterial.roughness = 0.3f;
			playerMaterial.emissive = {0.2f, 0.4f, 1.0f};
			playerMaterial.emissiveStrength = 2.0f;
		}
		else
		{
			// 세 번째: 거친 표면
			playerMaterial.albedo = {0.3f, 0.9f, 0.3f};
			playerMaterial.metallic = 0.0f;
			playerMaterial.roughness = 0.9f;
			playerMaterial.emissive = {0.0f, 0.0f, 0.0f};
			playerMaterial.emissiveStrength = 0.0f;
		}
		m_materials.push_back(playerMaterial);
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

	auto* device5 = reinterpret_cast<ID3D12Device5*>(device.GetDevice());
	auto* cmdList4 = reinterpret_cast<ID3D12GraphicsCommandList4*>(cmdList);

	for (auto& actor : m_sceneActors)
	{
		auto* mesh = actor->GetComponent<MeshComponent>();
		if (mesh)
		{
			mesh->BuildBLAS(device5, cmdList4, uploadHeap);
		}
	}

	std::vector<Actor*> actorPtrs;
	actorPtrs.reserve(m_sceneActors.size());
	for (auto& actor : m_sceneActors)
	{
		actorPtrs.push_back(actor.get());
	}

	m_tlas = std::make_unique<DxTLAS>();
	m_tlas->Build(device5, cmdList4, uploadHeap, actorPtrs);

	frame->ExecuteAndSignal(commandQueue.GetQueue());
	frame->WaitForCompletion();

	DEBUG_LOG_FMT("[GameFramework] Acceleration structures built\n");
}

void GameFramework::CreateRaytracingPipeline()
{
	auto& device = MANAGER(DxDeviceGlobal);
	auto* device5 = reinterpret_cast<ID3D12Device5*>(device.GetDevice());

	m_rtPipeline = std::make_unique<DxRtPipelineState>();
	m_rtPipeline->Create(device5, L"../EisenValor/Resource/Shader/RaytracingLibrary.hlsl", 1);

	m_shaderTable = std::make_unique<DxRtShaderTable>();
	m_shaderTable->Build(device5, m_rtPipeline.get(), static_cast<uint32_t>(m_sceneActors.size()));

	DEBUG_LOG_FMT("[GameFramework] Raytracing pipeline created\n");
}

void GameFramework::CreateMaterialBuffer()
{
	auto& device = MANAGER(DxDeviceGlobal);
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	if (m_materials.empty())
	{
		DEBUG_LOG_FMT("[GameFramework] WARNING: No materials to upload\n");
		return;
	}

	const uint64_t bufferSize = m_materials.size() * sizeof(PBRMaterial);

	auto* frame = m_frameResources[0].get();
	frame->BeginFrame();

	auto* uploadHeap = frame->GetUploadHeap();
	auto  upload = uploadHeap->UploadRawData(m_materials.data(), bufferSize, 256);

	D3D12_RESOURCE_DESC bufferDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = bufferSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc{.Count = 1, .Quality = 0},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};

	D3D12_HEAP_PROPERTIES defaultHeap = {};
	defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&m_materialBuffer)
	));

	m_materialBuffer->SetName(L"MaterialBuffer");

	auto* cmdList = frame->GetMainContext()->CommandList();
	cmdList->CopyBufferRegion(m_materialBuffer.Get(), 0, uploadHeap->GetResource(), upload.offset, bufferSize);

	auto barrier = DxUtils::CreateTransitionBarrier(
		m_materialBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
	);
	cmdList->ResourceBarrier(1, &barrier);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(m_materials.size());
	srvDesc.Buffer.StructureByteStride = sizeof(PBRMaterial);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	m_materialBufferSRVIndex = descHeap.CreateSRV(device.GetDevice(), m_materialBuffer.Get(), &srvDesc);

	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	frame->ExecuteAndSignal(commandQueue.GetQueue());
	frame->WaitForCompletion();

	DEBUG_LOG_FMT(
		"[GameFramework] Material buffer created: {} materials, SRV Index: {}\n", m_materials.size(),
		m_materialBufferSRVIndex
	);
}

void GameFramework::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = MANAGER(DxDeviceGlobal);
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	D3D12_RESOURCE_DESC desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width = width,
		.Height = height,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc{.Count = 1, .Quality = 0},
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	};

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
		IID_PPV_ARGS(&m_raytracingOutput)
	));

	m_raytracingOutput->SetName(L"RaytracingOutput");

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	m_raytracingOutputUAVIndex = descHeap.CreateUAV(device.GetDevice(), m_raytracingOutput.Get(), &uavDesc);

	DEBUG_LOG_FMT(
		"[GameFramework] Raytracing output created: {}x{}, UAV Index={}\n", width, height, m_raytracingOutputUAVIndex
	);
}

void GameFramework::ResizeRaytracingResources(uint32_t width, uint32_t height)
{
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	commandQueue.WaitForIdle();

	if (m_raytracingOutputUAVIndex != ~0u)
	{
		descHeap.FreeImmediate(m_raytracingOutputUAVIndex);
	}

	m_raytracingOutput.Reset();
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
}

void GameFramework::Release()
{
	DEBUG_LOG_FMT("WaitForIdle....\n");
	MANAGER(DxGfxCommandQueueGlobal).WaitForIdle();
	if (m_materialBufferSRVIndex != ~0u)
	{
		auto& descHeap = MANAGER(DxDescriptorHeapGlobal);
		descHeap.FreeImmediate(m_materialBufferSRVIndex);
	}
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

	const float dt = MANAGER(TimerGlobal).GetDeltaTime();

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
	auto* cmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(frame->GetMainContext()->CommandList());
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	RenderDXR();

	// RT Output → BackBuffer copy
	auto backBuffer = m_swapChain->GetCurrentBackBuffer();

	auto barrier1 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	auto barrier2 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_RESOURCE_BARRIER barriers[] = {barrier1, barrier2};
	cmdList->ResourceBarrier(2, barriers);

	cmdList->CopyResource(backBuffer, m_raytracingOutput.Get());

	// BackBuffer → Present, RT Output → UAV
	auto barrier3 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	auto barrier4 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	D3D12_RESOURCE_BARRIER restoreBarriers[] = {barrier3, barrier4};
	cmdList->ResourceBarrier(2, restoreBarriers);

	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	frame->ExecuteAndSignal(commandQueue.GetQueue());

	m_swapChain->PresentMaxPerformance();
}

void GameFramework::RenderDXR()
{
	auto* frame = m_frameResources[m_currentFrameIndex].get();
	auto* cmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(frame->GetMainContext()->CommandList());
	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	cmdList->SetPipelineState1(m_rtPipeline->GetStateObject());
	cmdList->SetComputeRootSignature(m_rtPipeline->GetGlobalRootSignature());

	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);

	// Param 0: TLAS (SRV)
	D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV = descHeap.GetGPUHandle(m_tlas->GetSRVIndex());
	cmdList->SetComputeRootDescriptorTable(0, tlasSRV);

	// Param 1: Output UAV
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV = descHeap.GetGPUHandle(m_raytracingOutputUAVIndex);
	cmdList->SetComputeRootDescriptorTable(1, outputUAV);

	// Param 2: Camera Constants (임시 ViewProjInverse)
	DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat3(&m_cameraPosition);
	DirectX::XMVECTOR forwardVec = DirectX::XMLoadFloat3(&m_cameraForward);
	DirectX::XMVECTOR upVec = DirectX::XMLoadFloat3(&m_cameraUp);
	DirectX::XMVECTOR targetVec = DirectX::XMVectorAdd(cameraPos, forwardVec);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(cameraPos, targetVec, upVec);

	float			  aspectRatio = static_cast<float>(m_swapChain->GetWidth()) / m_swapChain->GetHeight();
	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, aspectRatio, 0.1f, 1000.0f);

	DirectX::XMMATRIX viewProj = view * proj;
	DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse(nullptr, viewProj);

	DirectX::XMFLOAT4X4 viewProjInvFloat;
	DirectX::XMStoreFloat4x4(&viewProjInvFloat, viewProjInverse);

	cmdList->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);

	// Param 3: Materials (SRV)
	D3D12_GPU_DESCRIPTOR_HANDLE materialsSRV = descHeap.GetGPUHandle(m_materialBufferSRVIndex);
	cmdList->SetComputeRootDescriptorTable(3, materialsSRV);

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
	if (input.GetInputUp(VK_RBUTTON))
	{
		m_cameraEnabled = false;
		ShowCursor(TRUE); // 마우스 커서 보이기
	}

	if (!m_cameraEnabled)
		return;

	// 키보드 입력으로 이동
	float			  velocity = m_cameraSpeed * deltaTime;
	DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&m_cameraPosition);
	DirectX::XMVECTOR forwardVec = DirectX::XMLoadFloat3(&m_cameraForward);
	DirectX::XMVECTOR rightVec = DirectX::XMLoadFloat3(&m_cameraRight);
	DirectX::XMFLOAT3 worldUp = {0.0f, 1.0f, 0.0f};
	DirectX::XMVECTOR upVec = DirectX::XMLoadFloat3(&worldUp);

	// W/S: 전진/후진
	if (input.GetInput('W'))
		posVec = DirectX::XMVectorAdd(posVec, DirectX::XMVectorScale(forwardVec, velocity));
	if (input.GetInput('S'))
		posVec = DirectX::XMVectorSubtract(posVec, DirectX::XMVectorScale(forwardVec, velocity));

	// A/D: 좌/우 이동
	if (input.GetInput('A'))
		posVec = DirectX::XMVectorSubtract(posVec, DirectX::XMVectorScale(rightVec, velocity));
	if (input.GetInput('D'))
		posVec = DirectX::XMVectorAdd(posVec, DirectX::XMVectorScale(rightVec, velocity));

	// SHIFT/SPACE: 상/하 이동
	if (input.GetInput(VK_SHIFT))
		posVec = DirectX::XMVectorSubtract(posVec, DirectX::XMVectorScale(upVec, velocity));
	if (input.GetInput(VK_SPACE))
		posVec = DirectX::XMVectorAdd(posVec, DirectX::XMVectorScale(upVec, velocity));

	DirectX::XMStoreFloat3(&m_cameraPosition, posVec);

	// 마우스 이동으로 회전 (우클릭 중일 때만)
	auto mousePosition = input.GetMousePosition();

	if (m_firstMouse)
	{
		m_lastMouseX = mousePosition.x;
		m_lastMouseY = mousePosition.y;
		m_firstMouse = false;
	}

	float xOffset = static_cast<float>(mousePosition.x - m_lastMouseX);
	float yOffset = static_cast<float>(m_lastMouseY - mousePosition.y); // Y는 반대

	m_lastMouseX = mousePosition.x;
	m_lastMouseY = mousePosition.y;

	xOffset *= m_mouseSensitivity;
	yOffset *= m_mouseSensitivity;

	m_cameraYaw += xOffset;
	m_cameraPitch += yOffset;

	// Pitch 제한 (상하 90도 제한)
	if (m_cameraPitch > 89.0f)
		m_cameraPitch = 89.0f;
	if (m_cameraPitch < -89.0f)
		m_cameraPitch = -89.0f;

	UpdateCameraVectors();
}

void GameFramework::UpdateCameraVectors()
{
	// Forward 벡터 계산
	DirectX::XMFLOAT3 forward;
	forward.x = cosf(DirectX::XMConvertToRadians(m_cameraYaw)) * cosf(DirectX::XMConvertToRadians(m_cameraPitch));
	forward.y = sinf(DirectX::XMConvertToRadians(m_cameraPitch));
	forward.z = sinf(DirectX::XMConvertToRadians(m_cameraYaw)) * cosf(DirectX::XMConvertToRadians(m_cameraPitch));

	DirectX::XMVECTOR forwardVec = DirectX::XMLoadFloat3(&forward);
	forwardVec = DirectX::XMVector3Normalize(forwardVec);
	DirectX::XMStoreFloat3(&m_cameraForward, forwardVec);

	// Right 벡터 계산
	DirectX::XMFLOAT3 worldUp = {0.0f, 1.0f, 0.0f};
	DirectX::XMVECTOR worldUpVec = DirectX::XMLoadFloat3(&worldUp);
	DirectX::XMVECTOR rightVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forwardVec, worldUpVec));
	DirectX::XMStoreFloat3(&m_cameraRight, rightVec);

	// Up 벡터 계산
	DirectX::XMVECTOR upVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(rightVec, forwardVec));
	DirectX::XMStoreFloat3(&m_cameraUp, upVec);
}
