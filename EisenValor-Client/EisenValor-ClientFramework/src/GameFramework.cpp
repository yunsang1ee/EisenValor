#include "stdafxClientFramework.h"
#include "GameFramework.h"
#include "GlobalInterfaces.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxUtils.h"
#include "DxFeatureCaps.h"
#include "Vertex.h"
#include "GameObjectManager.h"
#include "LocalPlayer.h"
#include "Ground.h"
#include "NPC.h"

using namespace DirectX;
#define SERVER

bool GameFramework::Initialize(HINSTANCE hInstance, HWND hwnd)
{
#ifdef SERVER
	NetBridge::ServerPacketHandler::Init();

	if (false == MANAGER(NetBridge::NetworkManager)->Init())
		return false;
#endif
	m_hInstance = hInstance;
	m_hWnd = hwnd;

	Globals::InitializeGlobalRegistry();
	Globals::Timer().Initialize();
	Globals::Timer().SetFixedFPS(60);
	Globals::Timer().SetTargetFPS(144);

	// 1. 스왑체인 생성 코드 추가 25.07.19
	// RTV 디스크립터 힙 생성
	auto& device = GlobalRegistry::Get<IDxDeviceGlobal>();
	m_featureCaps = DxFeatureCaps::Query(device.GetDevice(), device.GetAdapter());
	m_featureCaps.LogCapabilities();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 3; // 백버퍼 3개
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device.GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
	m_rtvDescriptorSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 윈도우 크기 가져오기
	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);
	uint32_t width = clientRect.right - clientRect.left;
	uint32_t height = clientRect.bottom - clientRect.top;

	// 스왑체인 생성
	auto& commandQueue = GlobalRegistry::Get<IDxGraphicsCommandQueueGlobal>();

	m_swapChain = std::make_unique<DxSwapChain>(
		device.GetDevice(), device.GetFactory(), commandQueue, m_hWnd, width, height,
		3, // 백버퍼 개수
		DXGI_FORMAT_R8G8B8A8_UNORM, m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_rtvDescriptorSize
	);

	// 커맨드 컨텍스트 풀 생성
	m_commandContextPool = std::make_unique<DxCommandContextPool>(
		device.GetDevice(), commandQueue,
		3 // 백버퍼 개수
	);


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
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(device.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	if (m_featureCaps.supportsRayTracing && m_featureCaps.rayTracingTier >= D3D12_RAYTRACING_TIER_1_0)
	{
		DEBUG_LOG_FMT("[GameFramework] DXR supported. Initializing raytracing pipeline.\n");
		InitializeRaytracing();
	}
	else
	{
		DEBUG_LOG_FMT("[GameFramework] DXR not supported. Using rasterization pipeline.\n");
	}

	std::string id, pw;
	std::cout << "Input ID(any):";
	std::cin >> id;
	id = "ID";
	std::cout << "\n";
	std::cout << "Input PW(any):";
	std::cin >> pw;
	pw = "PW";

	auto pb = NetBridge::ServerPacketHandler::Make_CS_LOGIN_PACKET(id.c_str(), pw.c_str());
	MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));

	// Ground 객체 생성 및 초기화
	m_ground = std::make_unique<Ground>();
	m_ground->Initialize(device.GetDevice());

	// Player 객체 생성 및 초기화 추가
	// auto player = std::make_unique<Player>();
	// player->SetPosition(0.0f, 0.5f, 0.0f);  // 초기 위치 설정
	// player->Initialize(device.GetDevice());
	// m_player = player.get();

	// Objects들 추가
	//  m_gameObjects.push_back(std::move(player));

	// 적군 장수 생성
	// auto enemyGeneral = std::make_shared<NPC>();
	// enemyGeneral->SetTeam(NPC::Team::ENEMY);
	// enemyGeneral->SetUnitType(NPC::UnitType::GENERAL);
	// enemyGeneral->Initialize(device.GetDevice());
	// Vec3 generalPos(0.0f, 0.0f, 8.0f);
	// enemyGeneral->SetPosition(generalPos);
	// enemyGeneral->lastServerPosition = generalPos;
	// MANAGER(GameObjectManager)->AddObject(enemyGeneral);

	// 적군 병사 생성 (장수 바로 옆에)
	// auto enemySoldier = std::make_shared<NPC>();
	// enemySoldier->SetTeam(NPC::Team::ENEMY);
	// enemySoldier->SetUnitType(NPC::UnitType::SOLDIER);
	// enemySoldier->Initialize(device.GetDevice());
	// Vec3 soldierPos(1.5f, 0.0f, 8.0f);
	// enemySoldier->SetPosition(soldierPos);
	// enemySoldier->lastServerPosition = soldierPos;
	// MANAGER(GameObjectManager)->AddObject(enemySoldier);

	// 아군 배틀램 생성
	// auto battleram = std::make_shared<NPC>();
	// battleram->SetTeam(NPC::Team::ALLY);
	// battleram->SetUnitType(NPC::UnitType::BATTLE_RAM);
	// battleram->Initialize(device.GetDevice());
	// Vec3 battleramPos(-3.0f, 0.0f, 2.0f);
	// battleram->SetPosition(battleramPos);
	// battleram->lastServerPosition = battleramPos;
	// MANAGER(GameObjectManager)->AddObject(battleram);

	// 아군 스폰 기지 생성
	// auto allyspawnbase = std::make_shared<NPC>();
	// allyspawnbase->SetTeam(NPC::Team::ALLY);
	// allyspawnbase->SetUnitType(NPC::UnitType::SPAWN_BASE);
	// allyspawnbase->Initialize(device.GetDevice());
	// Vec3 spawnbasePos(-8.0f, 0.0f, 0.0f);
	// allyspawnbase->SetPosition(spawnbasePos);
	// allyspawnbase->lastServerPosition = spawnbasePos;
	// MANAGER(GameObjectManager)->AddObject(allyspawnbase);

	return true;
}

void GameFramework::Run()
{
#ifdef SERVER
	MANAGER(NetBridge::NetworkManager)->ProcessIO();
#endif

	Globals::Input().BeforeUpdate();

	Globals::Timer().Update();
	if (Globals::Timer().ShouldFixedUpdate())
		FixedUpdate();
	Update();
	LateUpdate();

	Render();

	Globals::Input().AfterUpdate();
	MANAGER(GameObjectManager)->FinalUpdate();
}

void GameFramework::Release()
{
	DEBUG_LOG_FMT("WaitForIdle....\n");
	GlobalRegistry::Get<IDxGraphicsCommandQueueGlobal>().WaitForIdle();
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
			Globals::Input().OnInputState(code, isPressed, isUp);
	}
	break;
	case WM_MOUSEMOVE:
		Globals::Input().OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
	{
		Globals::Input().OnInputState(VK_LBUTTON, FALSE, FALSE);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		Globals::Input().OnInputState(VK_RBUTTON, FALSE, FALSE);
		break;
	}
	case WM_LBUTTONUP:
	{
		Globals::Input().OnInputState(VK_LBUTTON, FALSE, TRUE);
		break;
	}
	case WM_RBUTTONUP:
	{
		Globals::Input().OnInputState(VK_RBUTTON, FALSE, TRUE);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		Globals::Input().OnWheelScroll(GET_WHEEL_DELTA_WPARAM(wParam));
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
	auto& input = Globals::Input();
	if (input.GetInputDown(VK_ESCAPE))
	{
		DEBUG_LOG_FMT("close\n");
		::DestroyWindow(m_hWnd);
	}
	if (input.GetInputDown(VK_F11))
	{
		m_swapChain->ToggleBorderlessFullscreen();
	}
	if (input.GetInput(VK_MENU) && input.GetInputDown(VK_RETURN))
	{
		m_swapChain->ToggleFullscreen();
	}

	const float dt = Globals::Timer().GetDeltaTime();

	MANAGER(GameObjectManager)->Update(dt);
}

void GameFramework::FixedUpdate() {}

void GameFramework::LateUpdate() {}

// Render 코드 생성 25.07.20
void GameFramework::Render()
{
	auto localPlayer = MANAGER(GameObjectManager)->GetLocalPlayer();
	if (localPlayer == nullptr)
		return;

	// 현재 프레임 준비
	m_commandContextPool->AdvanceFrame();
	auto& context = m_commandContextPool->GetCurrentContext();

	const XMMATRIX view = localPlayer->GetViewMatrix();

	//// 투영 행렬
	XMMATRIX projection = XMMatrixPerspectiveFovLH(
		XM_PI / 4.0f,											   // 45도 시야각
		(float)m_swapChain->GetWidth() / m_swapChain->GetHeight(), // 종횡비
		0.1f,													   // 가까운 클리핑 평면
		100.0f													   // 먼 클리핑 평면
	);

	// 현재 백버퍼 가져오기
	auto rtvHandle = m_swapChain->GetCurrentBackBufferRTV();
	auto backBuffer = m_swapChain->GetCurrentBackBuffer();

	// 백버퍼를 렌더 타겟으로 전환(Resource barrier)
	auto barrier =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	context.CommandList()->ResourceBarrier(1, &barrier);

	// 렌더 타겟 설정
	context.CommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	// 화면을 파란색으로 클리어
	float clearColor[] = {0.0f, 0.0f, 1.0f, 1.0f}; // 파란색
	context.CommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// 뷰포트 설정
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(m_swapChain->GetWidth());
	viewport.Height = static_cast<float>(m_swapChain->GetHeight());
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	D3D12_RECT scissorRect = {};
	scissorRect.right = static_cast<LONG>(m_swapChain->GetWidth());
	scissorRect.bottom = static_cast<LONG>(m_swapChain->GetHeight());
	context.CommandList()->RSSetViewports(1, &viewport);
	context.CommandList()->RSSetScissorRects(1, &scissorRect);

	// 파이프라인 설정
	context.CommandList()->SetGraphicsRootSignature(m_rootSignature.Get());
	context.CommandList()->SetPipelineState(m_pipelineState.Get());

	// Ground 렌더링 (임시)
	const auto cmdList = context.CommandList();
	m_ground->Render(cmdList, view, projection);

	// GameObject 렌더링 (임시)
	MANAGER(GameObjectManager)->Render(cmdList, view, projection);

	// 백버퍼를 프레젠트 상태로 전환
	barrier =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	context.CommandList()->ResourceBarrier(1, &barrier);

	// 커맨드 실행
	m_commandContextPool->SignalCurrentFrame();

	m_swapChain->PresentMaxPerformance();
}

void GameFramework::InitializeRaytracing()
{
	auto& device = GlobalRegistry::Get<IDxDeviceGlobal>();
	// 1. DXR 디바이스 인터페이스 가져오기
	ThrowIfFailed(device.GetDevice()->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)));

	// 2. DXR 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 10,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		.NodeMask = 0
	};
	ThrowIfFailed(m_dxrDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_raytracingDescriptorHeap)));
	// 3. 출력 텍스처 생성
	CreateRaytracingOutputTexture();
	// 4. 루트 시그니처 생성
	CreateRaytracingRootSignatures();
	// 5. 파이프라인 상태 객체(PSO) 생성
	CreateRaytracingPipelineStateObject();
	// 6. 가속 구조 생성
	BuildAccelerationStructures();
	// 7. 셰이더 테이블 생성
	CreateShaderTable();
	DEBUG_LOG_FMT("[GameFramework] Raytracing initialization completed.\n");
}

void GameFramework::CreateRaytracingOutputTexture()
{
	auto width = m_swapChain->GetWidth();
	auto height = m_swapChain->GetHeight();

	D3D12_RESOURCE_DESC desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width = width,
		.Height = height,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc{.Count = 1},
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	};

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	ThrowIfFailed(m_dxrDevice->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
		IID_PPV_ARGS(&m_raytracingOutput)
	));

	DxUtils::SetDebugName(m_raytracingOutput.Get(), L"RaytracingOutput");
}

void GameFramework::CreateRaytracingRootSignatures()
{
	D3D_ROOT_SIGNATURE_VERSION targetVersion = std::min(
		m_featureCaps.rootSignature.HighestVersion,
		D3D_ROOT_SIGNATURE_VERSION_1_1 // 1.1로 제한
	);
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = {};
	versionedDesc.Version = targetVersion;

	constexpr auto kAS = std::to_underlying(ERTGlobalRootSignatureSlot::AccelerationStructureSlot);
	constexpr auto kOutputUAV = std::to_underlying(ERTGlobalRootSignatureSlot::OutputViewSlot);
	constexpr auto kCameraCB = std::to_underlying(ERTGlobalRootSignatureSlot::CameraConstantSlot);

	if (targetVersion == D3D_ROOT_SIGNATURE_VERSION_1_1)
	{
		D3D12_ROOT_PARAMETER1	params[3] = {};
		D3D12_DESCRIPTOR_RANGE1 ranges[2] = {};
		// SRV table: TLAS at t0, space0
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
		ranges[0].OffsetInDescriptorsFromTableStart = 0;

		// UAV table: Output RWTexture2D at u0, space0
		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[1].NumDescriptors = 1;
		ranges[1].BaseShaderRegister = 0;
		ranges[1].RegisterSpace = 0;
		ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		ranges[1].OffsetInDescriptorsFromTableStart = 0;

		// Table 0: SRV(TLAS)
		params[kAS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[kAS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // RT는 ALL 사용
		params[kAS].DescriptorTable.NumDescriptorRanges = 1;
		params[kAS].DescriptorTable.pDescriptorRanges = &ranges[0];

		// Table 1: UAV(Output)
		params[kOutputUAV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[kOutputUAV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		params[kOutputUAV].DescriptorTable.NumDescriptorRanges = 1;
		params[kOutputUAV].DescriptorTable.pDescriptorRanges = &ranges[1];

		// Root CBV: Camera
		params[kCameraCB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[kCameraCB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		params[kCameraCB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		params[kCameraCB].Descriptor.ShaderRegister = 0; // b0
		params[kCameraCB].Descriptor.RegisterSpace = 0;	 // space0

		versionedDesc.Desc_1_1.NumParameters = _countof(params);
		versionedDesc.Desc_1_1.pParameters = params;
		versionedDesc.Desc_1_1.NumStaticSamplers = 0;
		versionedDesc.Desc_1_1.pStaticSamplers = nullptr;
		versionedDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	}
	else
	{ // Root Signature 1.0 사용
		D3D12_ROOT_PARAMETER params[3] = {};

		constexpr auto kAS = std::to_underlying(ERTGlobalRootSignatureSlot::AccelerationStructureSlot);
		constexpr auto kOutputView = std::to_underlying(ERTGlobalRootSignatureSlot::OutputViewSlot);
		constexpr auto kCamera = std::to_underlying(ERTGlobalRootSignatureSlot::CameraConstantSlot);

		// Scene (TLAS) t0
		params[kAS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		params[kAS].Descriptor.ShaderRegister = 0;
		params[kAS].Descriptor.RegisterSpace = 0;
		params[kAS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// Output texture u0
		params[kOutputView].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		params[kOutputView].Descriptor.ShaderRegister = 0;
		params[kOutputView].Descriptor.RegisterSpace = 0;
		params[kOutputView].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// Camera constants b0
		params[kCamera].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[kCamera].Descriptor.ShaderRegister = 0;
		params[kCamera].Descriptor.RegisterSpace = 0;
		params[kCamera].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		versionedDesc.Desc_1_0.NumParameters = _countof(params);
		versionedDesc.Desc_1_0.pParameters = params;
		versionedDesc.Desc_1_0.NumStaticSamplers = 0;
		versionedDesc.Desc_1_0.pStaticSamplers = nullptr;
		versionedDesc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	}


	ComPtr<ID3DBlob> signature, error;
	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&versionedDesc, &signature, &error));
	ThrowIfFailed(m_dxrDevice->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rtGlobalRootSignature)
	));
}

void GameFramework::CreateRaytracingPipelineStateObject()
{
	// 셰이더 컴파일
	ComPtr<IDxcUtils>		   dxcUtils;
	ComPtr<IDxcCompiler3>	   dxcCompiler;
	ComPtr<IDxcIncludeHandler> includeHandler;

	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler)));
	ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));

	// Raytracing Library
	ComPtr<IDxcBlob> raytracingLibrary = CompileShader(
		dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(),
		L"../EisenValor/Resource/Shader/RaytracingLibrary.hlsl", L"", L"lib_6_3" // Gen + Miss + ClosestHit
	);

	D3D12_EXPORT_DESC exports[] = {
		{L"RayGenMain", nullptr, D3D12_EXPORT_FLAG_NONE},
		{L"MissMain", nullptr, D3D12_EXPORT_FLAG_NONE},
		{L"ClosestHitMain", nullptr, D3D12_EXPORT_FLAG_NONE}
	};

	std::vector<D3D12_STATE_SUBOBJECT> subObjects;

	// 1. DXIL Library
	D3D12_DXIL_LIBRARY_DESC dxilLib = {
		.DXILLibrary = {raytracingLibrary->GetBufferPointer(), raytracingLibrary->GetBufferSize()},
		.NumExports = _countof(exports),
		.pExports = exports
	};
	D3D12_STATE_SUBOBJECT dxilLibSO = {.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &dxilLib};
	subObjects.push_back(dxilLibSO);

	// 2. Hit Group
	D3D12_HIT_GROUP_DESC hitGroupDesc = {
		.HitGroupExport = L"HitGroup",
		.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
		.AnyHitShaderImport = nullptr,
		.ClosestHitShaderImport = L"ClosestHitMain",
		.IntersectionShaderImport = nullptr
	};

	D3D12_STATE_SUBOBJECT hitGroupSO = {.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &hitGroupDesc};
	subObjects.push_back(hitGroupSO);

	// 3. Shader Config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {
		.MaxPayloadSizeInBytes = sizeof(float) * 4, .MaxAttributeSizeInBytes = sizeof(float) * 2
	};

	D3D12_STATE_SUBOBJECT shaderConfigSO = {
		.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, .pDesc = &shaderConfig
	};
	subObjects.push_back(shaderConfigSO);

	// 4. Global Root Signature
	D3D12_STATE_SUBOBJECT globalRootSignatureSO = {
		.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = m_rtGlobalRootSignature.Get()
	};
	subObjects.push_back(globalRootSignatureSO);

	// 5. Pipeline Config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {.MaxTraceRecursionDepth = 2};

	D3D12_STATE_SUBOBJECT pipelineConfigSO = {
		.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, .pDesc = &pipelineConfig
	};
	subObjects.push_back(pipelineConfigSO);

	D3D12_STATE_OBJECT_DESC raytracingPipelineDesc = {
		.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.NumSubobjects = static_cast<uint32>(subObjects.size()),
		.pSubobjects = subObjects.data()
	};
	ThrowIfFailed(m_dxrDevice->CreateStateObject(&raytracingPipelineDesc, IID_PPV_ARGS(&m_rtStateObject)));

	ThrowIfFailed(m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps)));

	CreateShaderTable();

	DEBUG_LOG_FMT(
		"[GameFramework] Raytracing pipeline state object created successfully with {} subobjects.\n", subObjects.size()
	);
}

void GameFramework::CreateShaderTable()
{
	constexpr uint32_t shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	constexpr uint32_t shaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

	uint32_t rayGenEntrySize = shaderIdentifierSize;
	uint32_t missEntrySize = shaderIdentifierSize;
	uint32_t hitGroupEntrySize = shaderIdentifierSize;
	uint32_t shaderTableSize = rayGenEntrySize + missEntrySize + hitGroupEntrySize;
	// 셰이더 테이블 버퍼 생성
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = shaderTableSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	auto& device = GlobalRegistry::Get<IDxDeviceGlobal>();
	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_shaderTable)
	));
	DxUtils::SetDebugName(m_shaderTable.Get(), L"ShaderTable");
	// 셰이더 테이블에 데이터 기록
	uint8_t*	pData;
	D3D12_RANGE readRange = {0, 0};
	ThrowIfFailed(m_shaderTable->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
	// RayGen 셰이더 ID
	void* pRayGenShaderID = m_rtStateObjectProps->GetShaderIdentifier(L"RayGenMain");
	memcpy(pData, pRayGenShaderID, shaderIdentifierSize);
	pData += rayGenEntrySize;
	// Miss 셰이더 ID
	void* pMissShaderID = m_rtStateObjectProps->GetShaderIdentifier(L"MissMain");
	memcpy(pData, pMissShaderID, shaderIdentifierSize);
	pData += missEntrySize;
	// Hit Group 셰이더 ID
	void* pHitGroupShaderID = m_rtStateObjectProps->GetShaderIdentifier(L"HitGroup");
	memcpy(pData, pHitGroupShaderID, shaderIdentifierSize);
	m_shaderTable->Unmap(0, nullptr);
}

void GameFramework::BuildAccelerationStructures()
{
	auto& device = GlobalRegistry::Get<IDxDeviceGlobal>();
	auto& commandQueue = GlobalRegistry::Get<IDxGraphicsCommandQueueGlobal>();

	auto& context = m_commandContextPool->GetCurrentContext();

	ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;
	ThrowIfFailed(context.CommandList()->QueryInterface(IID_PPV_ARGS(&dxrCommandList)));

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances;
	if (m_ground)
	{
		m_ground->BuildAccelerationStructure(m_dxrDevice.Get(), dxrCommandList.Get());
		D3D12_RAYTRACING_INSTANCE_DESC groundInstance = {};
		groundInstance.Transform[0][0] = 1.0f;
		groundInstance.Transform[1][1] = 1.0f;
		groundInstance.Transform[2][2] = 1.0f;
		groundInstance.InstanceID = RaytracingInstanceID::kGround;
		groundInstance.InstanceMask = 0xFF;
		groundInstance.AccelerationStructure = m_ground->GetBottomLevelAS()->GetGPUVirtualAddress();
		instances.push_back(groundInstance);
	}

	auto localPlayer = MANAGER(GameObjectManager)->GetLocalPlayer();
	if (localPlayer)
	{
		localPlayer->BuildAccelerationStructure(m_dxrDevice.Get(), dxrCommandList.Get());

		D3D12_RAYTRACING_INSTANCE_DESC localPlayerInstance = {};
		auto						   transform = localPlayer->GetTransform();
		memcpy(localPlayerInstance.Transform, &transform, sizeof(localPlayerInstance.Transform));
		localPlayerInstance.InstanceID = RaytracingInstanceID::kLocalPlayer;
		localPlayerInstance.InstanceMask = 0xFF;
		localPlayerInstance.AccelerationStructure = localPlayer->GetBottomLevelAS()->GetGPUVirtualAddress();
		instances.push_back(localPlayerInstance);
	}
	auto gameObjects = MANAGER(GameObjectManager)->GetAllObjects();
	for (auto& obj : gameObjects)
	{
		if (obj.get()->GetRaytracingInstanceType() == RaytracingInstanceType::Player)
		{
			auto player = std::static_pointer_cast<Player>(obj);
			player->BuildAccelerationStructure(m_dxrDevice.Get(), dxrCommandList.Get());

			D3D12_RAYTRACING_INSTANCE_DESC instance = {};
			auto						   transform = player->GetTransform();
			memcpy(instance.Transform, &transform, sizeof(instance.Transform));
			instance.InstanceID = instanceId++;
			instance.InstanceMask = 0xFF;
			instance.AccelerationStructure = player->GetBottomLevelAS()->GetGPUVirtualAddress();
			instances.push_back(instance);
		}
	}

	BuildTopLevelAS(dxrCommandList.Get(), instances);

	ThrowIfFailed(dxrCommandList->Close());
	ID3D12CommandList* cmdLists[] = {dxrCommandList.Get()};
	commandQueue.GetQueue()->ExecuteCommandLists(1, cmdLists);
	commandQueue.WaitForIdle();

	DEBUG_LOG_FMT("[GameFramework] Acceleration structures built successfully\n");
}

void GameFramework::BuildTopLevelAS(
	ID3D12GraphicsCommandList4* commandList, const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instances
)
{
	auto& device = GlobalRegistry::Get<IDxDeviceGlobal>();

	size_t instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instances.size();

	D3D12_HEAP_PROPERTIES uploadHeapProps = {};
	uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC instanceBufferDesc = {};
	instanceBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	instanceBufferDesc.Width = instanceBufferSize;
	instanceBufferDesc.Height = 1;
	instanceBufferDesc.DepthOrArraySize = 1;
	instanceBufferDesc.MipLevels = 1;
	instanceBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	instanceBufferDesc.SampleDesc.Count = 1;
	instanceBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> instanceBuffer;
	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &instanceBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&instanceBuffer)
	));

	void* mappedData;
	instanceBuffer->Map(0, nullptr, &mappedData);
	memcpy(mappedData, instances.data(), instanceBufferSize);
	instanceBuffer->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = static_cast<UINT>(instances.size());
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.InstanceDescs = instanceBuffer->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

	D3D12_HEAP_PROPERTIES defaultHeapProps = {};
	defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC tlasBufferDesc = {};
	tlasBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	tlasBufferDesc.Width = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
	tlasBufferDesc.Height = 1;
	tlasBufferDesc.DepthOrArraySize = 1;
	tlasBufferDesc.MipLevels = 1;
	tlasBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	tlasBufferDesc.SampleDesc.Count = 1;
	tlasBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	tlasBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &tlasBufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&m_topLevelAS)
	));

	// 스크래치 버퍼 생성
	ComPtr<ID3D12Resource> scratchBuffer;
	D3D12_RESOURCE_DESC	   scratchBufferDesc = {};
	scratchBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	scratchBufferDesc.Width = topLevelPrebuildInfo.ScratchDataSizeInBytes;
	scratchBufferDesc.Height = 1;
	scratchBufferDesc.DepthOrArraySize = 1;
	scratchBufferDesc.MipLevels = 1;
	scratchBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	scratchBufferDesc.SampleDesc.Count = 1;
	scratchBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	scratchBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &scratchBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
		IID_PPV_ARGS(&scratchBuffer)
	));

	// TLAS 빌드 설명 설정
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	topLevelBuildDesc.Inputs = topLevelInputs;
	topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAS->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();

	// TLAS 빌드
	commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	// UAV 배리어 설정
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_topLevelAS.Get();
	commandList->ResourceBarrier(1, &uavBarrier);

	DxUtils::SetDebugName(m_topLevelAS.Get(), L"TopLevelAS");
}

ComPtr<IDxcBlob> GameFramework::CompileShader(
	IDxcUtils*			dxcUtils,
	IDxcCompiler3*		dxcCompiler,
	IDxcIncludeHandler* includeHandler,
	LPCWSTR				fileName,
	LPCWSTR				entryPoint,
	LPCWSTR				target
)
{
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ThrowIfFailed(dxcUtils->LoadFile(fileName, nullptr, &sourceBlob));

	std::vector<LPCWSTR> arguments;
	arguments.push_back(L"-E");
	arguments.push_back(entryPoint);
	arguments.push_back(L"-T");
	arguments.push_back(target);

#ifdef _DEBUG
	arguments.push_back(L"-Zi");
	arguments.push_back(L"-Od");
#else
	arguments.push_back(L"-O3");
#endif

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = 0;

	ComPtr<IDxcResult> result;
	ThrowIfFailed(dxcCompiler->Compile(
		&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()), includeHandler, IID_PPV_ARGS(&result)
	));

	HRESULT hrr;
	result->GetStatus(&hrr);
	if (FAILED(hrr))
	{
		ComPtr<IDxcBlobEncoding> errorBlob;
		result->GetErrorBuffer(&errorBlob);
		if (errorBlob)
		{
			std::string errorMsg(static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
			DEBUG_LOG_FMT("Shader compilation failed: %s\n", errorMsg.c_str());
		}
		ThrowIfFailed(hrr);
	}

	ComPtr<IDxcBlob>	 shaderBlob;
	ComPtr<IDxcBlobWide> outputName;
	ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), &outputName));
	return shaderBlob;
}
