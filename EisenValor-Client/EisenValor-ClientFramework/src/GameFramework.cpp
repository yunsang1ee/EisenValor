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
#include "Vertex.h"
#include "GameObjectManager.h"
#include "LocalPlayer.h"
#include "Ground.h"
#include "NPC.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxCommandContext.h"

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

	// TODO: BLAS/TLAS 생성
	// TODO: RT Pipeline State 생성
	// TODO: Shader Table 생성

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
	m_ground = std::make_unique<Ground>();
	m_ground->Initialize(device.GetDevice());

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

//void GameFramework::RecreateDepthStencilBuffer(uint32_t width, uint32_t height)
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
//}

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
	ID3D12GraphicsCommandList* cmdList = frame->GetMainContext()->CommandList();
	auto*					   uploadHeap = frame->GetUploadHeap();

	// TODO: DispatchRays()

	// RT Output → BackBuffer copy
	auto backBuffer = m_swapChain->GetCurrentBackBuffer();
	auto rtvHandle = m_swapChain->GetCurrentBackBufferRTV();

	// RT Output → Copy Source
	auto barrier1 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	// BackBuffer → Copy Dest
	auto barrier2 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_RESOURCE_BARRIER barriers[] = {barrier1, barrier2};
	cmdList->ResourceBarrier(2, barriers);

	cmdList->CopyResource(backBuffer, m_raytracingOutput.Get());

	// BackBuffer → Present
	auto barrier3 =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	// RT Output → UAV
	auto barrier4 = DxUtils::CreateTransitionBarrier(
		m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	D3D12_RESOURCE_BARRIER restoreBarriers[] = {barrier3, barrier4};
	cmdList->ResourceBarrier(2, restoreBarriers);

	// 커맨드 실행
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	frame->ExecuteAndSignal(commandQueue.GetQueue());

	m_swapChain->PresentMaxPerformance();
}
