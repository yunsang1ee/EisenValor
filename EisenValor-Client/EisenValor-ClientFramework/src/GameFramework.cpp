#include "stdafxClientFramework.h"
#include "GameFramework.h"
#include "GlobalInterfaces.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "Vertex.h"
#include "Player.h"



using namespace DirectX;
// #define SERVER


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

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 3;  // 백버퍼 3개
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
		device.GetDevice(),
		device.GetFactory(),
		commandQueue,
		m_hWnd,
		width,
		height,
		3, // 백버퍼 개수
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		m_rtvDescriptorSize
	);

	// 커맨드 컨텍스트 풀 생성
	m_commandContextPool = std::make_unique<DxCommandContextPool>(
		device.GetDevice(),
		commandQueue,
		3	//백버퍼 개수
	);


	// 2. 큐브
	// 정점 버퍼 생성
	// 땅용 데이터를 지역 변수로 정의
	Vertex groundVertices[] = {
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(0.5f, 0.3f, 0.1f, 1.0f) },
		{ DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), DirectX::XMFLOAT4(0.5f, 0.3f, 0.1f, 1.0f) },
		{ DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), DirectX::XMFLOAT4(0.5f, 0.3f, 0.1f, 1.0f) },
		{ DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(0.5f, 0.3f, 0.1f, 1.0f) },
		{ DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), DirectX::XMFLOAT4(0.4f, 0.2f, 0.0f, 1.0f) },
		{ DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), DirectX::XMFLOAT4(0.4f, 0.2f, 0.0f, 1.0f) },
		{ DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), DirectX::XMFLOAT4(0.4f, 0.2f, 0.0f, 1.0f) },
		{ DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), DirectX::XMFLOAT4(0.4f, 0.2f, 0.0f, 1.0f) }
	};

	uint16_t groundIndices[] = {
		0, 1, 2,  0, 2, 3,
		4, 6, 5,  4, 7, 6,
		0, 5, 1,  0, 4, 5,
		3, 2, 6,  3, 6, 7,
		1, 5, 6,  1, 6, 2,
		0, 3, 7,  0, 7, 4
	};

	// 기존의 cubeVertices.data()를 groundVertices로 변경
	const UINT vertexBufferSize = sizeof(groundVertices);

	// GPU에 업로드할 힙
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 버퍼 리소스 설명
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = vertexBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 정점 버퍼 생성
	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)
	));


	// 정점 데이터를 버퍼에 복사
	UINT8* pVertexDataBegin;
	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, groundVertices, sizeof(groundVertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// VertexBufferView 설정
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	// 인덱스 버퍼 크기 계산
	const UINT indexBufferSize = sizeof(groundIndices);

	// 인덱스 버퍼용 힙 속성 (정점 버퍼와 동일)
	D3D12_HEAP_PROPERTIES indexHeapProps = {};
	indexHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 인덱스 버퍼 리소스 설명
	D3D12_RESOURCE_DESC indexResourceDesc = {};
	indexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	indexResourceDesc.Width = indexBufferSize;
	indexResourceDesc.Height = 1;
	indexResourceDesc.DepthOrArraySize = 1;
	indexResourceDesc.MipLevels = 1;
	indexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	indexResourceDesc.SampleDesc.Count = 1;
	indexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 인덱스 버퍼 생성
	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&indexHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&indexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer)
	));

	// 인덱스 데이터를 버퍼에 복사
	UINT8* pIndexDataBegin;
	D3D12_RANGE indexReadRange = { 0, 0 };
	ThrowIfFailed(m_indexBuffer->Map(0, &indexReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, groundIndices, sizeof(groundIndices));
	m_indexBuffer->Unmap(0, nullptr);

	// Index Buffer View 설정
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = indexBufferSize;
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;  // 16비트 부호 없는 정수

	// 루트 파라미터 정의 (상수 버퍼용)
	D3D12_ROOT_PARAMETER rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 상수 버퍼 뷰
	rootParameter.Descriptor.ShaderRegister = 0;  // register(b0)
	rootParameter.Descriptor.RegisterSpace = 0;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 정점 셰이더에서만 사용

	// 3. 루트 시그니처 생성 25.07.20
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParameter;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	// 4. 셰이더 컴파일 (Simple)
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;


#ifdef _DEBUG
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	// 셰이더 파일에서 컴파일
	//ThrowIfFailed(D3DCompileFromFile(L"../../../EisenValor/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	//ThrowIfFailed(D3DCompileFromFile(L"../../../EisenValor/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
	// 셰이더 파일에서 컴파일 (올바른 경로로 수정)
	ThrowIfFailed(D3DCompileFromFile(L"Resources/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"Resources/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

	// 5. 입력 레이아웃 정의
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// PS 생성하기
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
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

	
	// 두 번째 상수 버퍼
	const UINT constantBufferSize2 = (sizeof(ConstantBuffer) + 255) & ~255;
	D3D12_HEAP_PROPERTIES cbHeapProps2 = {};
	cbHeapProps2.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC cbResourceDesc2 = {};
	cbResourceDesc2.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc2.Width = constantBufferSize2;
	cbResourceDesc2.Height = 1;
	cbResourceDesc2.DepthOrArraySize = 1;
	cbResourceDesc2.MipLevels = 1;
	cbResourceDesc2.Format = DXGI_FORMAT_UNKNOWN;
	cbResourceDesc2.SampleDesc.Count = 1;
	cbResourceDesc2.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&cbHeapProps2,
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc2,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer2)
	));

	D3D12_RANGE readRange2 = { 0, 0 };
	ThrowIfFailed(m_constantBuffer2->Map(0, &readRange2, reinterpret_cast<void**>(&m_pCbvDataBegin2)));

	// Player 객체 생성 및 초기화 추가
	auto player = std::make_unique<Player>();
	player->SetPosition(0.0f, 0.5f, 0.0f);  // 초기 위치 설정
	player->Initialize(device.GetDevice());


	m_player = player.get();

	m_gameObjects.push_back(std::move(player));

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
}

void GameFramework::Release()
{
}

LRESULT GameFramework::OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	WORD keyflags;
	bool isPressed;
	bool isUp;
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
		Globals::Input().OnWheelScroll(GET_WHEEL_DELTA_WPARAM(wParam));
		break;
	case WM_DESTROY:
		DEBUG_LOG_FMT("WaitForIdle....\n");
		GlobalRegistry::Get<IDxGraphicsCommandQueueGlobal>().WaitForIdle();
		DEBUG_LOG_FMT("Window destroyed. Initiating application shutdown.\n");
		PostQuitMessage(0);
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void GameFramework::Update()
{
	if (Globals::Input().GetInputDown(VK_ESCAPE))
	{
		DEBUG_LOG_FMT("close\n");
		::DestroyWindow(m_hWnd);
	}

	//모든 GameObject 업데이트
	for (auto& gameObject : m_gameObjects)
	{
		gameObject->Update(Globals::Timer().GetDeltaTime());
	}
}

void GameFramework::FixedUpdate()
{
}

void GameFramework::LateUpdate()
{
}


//Render 코드 생성 25.07.20
void GameFramework::Render()
{
	// 현재 프레임 준비
	m_commandContextPool->AdvanceFrame();
	auto& context = m_commandContextPool->GetCurrentContext();

	// MVP행렬 계산
	// 회전 애니메이션
	static float rotation = 0.0f;
	rotation += 0.01f; // 회전 속도


	// 월드 행렬
	XMMATRIX world = XMMatrixIdentity();

	// 뷰 행렬
	Player* player = static_cast<Player*>(m_player);
	XMMATRIX view = player->GetViewMatrix();

	// 투영 행렬
	XMMATRIX projection = XMMatrixPerspectiveFovLH(
		XM_PI / 4.0f,                                    // 45도 시야각
		(float)m_swapChain->GetWidth() / m_swapChain->GetHeight(), // 종횡비
		0.1f,                                           // 가까운 클리핑 평면
		100.0f                                          // 먼 클리핑 평면
	);


	// 현재 백버퍼 가져오기
	auto rtvHandle = m_swapChain->GetCurrentBackBufferRTV();
	auto backBuffer = m_swapChain->GetCurrentBackBuffer();

	// 백버퍼를 렌더 타겟으로 전환(Resource barrier)
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	context.CommandList()->ResourceBarrier(1, &barrier);

	// 렌더 타겟 설정
	context.CommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	// 화면을 파란색으로 클리어
	float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f }; // 파란색
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
	context.CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context.CommandList()->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	context.CommandList()->IASetIndexBuffer(&m_indexBufferView);

	// ===== 땅 그리기 =====
	XMMATRIX groundWorld = XMMatrixScaling(20.0f, 0.2f, 20.0f) *
		XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX groundMVP = groundWorld * view * projection;

	XMStoreFloat4x4(&m_constantBufferData2.mvp, XMMatrixTranspose(groundMVP));
	memcpy(m_pCbvDataBegin2, &m_constantBufferData2, sizeof(m_constantBufferData2));

	context.CommandList()->SetGraphicsRootConstantBufferView(0, m_constantBuffer2->GetGPUVirtualAddress());
	context.CommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);
	
	// 모든 GameObject 렌더링
	for (auto& gameObject : m_gameObjects)
	{
		gameObject->Render(context.CommandList(), view, projection);
	}

	// 백버퍼를 프레젠트 상태로 전환
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	context.CommandList()->ResourceBarrier(1, &barrier);


	// 커맨드 실행
	m_commandContextPool->SignalCurrentFrame();
	// 화면에 표시
	m_swapChain->Present(1, 0);

}std::vector<Vertex> cubeVertices = {
		// 앞면 정점들 (z = 0.5)
		{ { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 0: 왼쪽 위 앞 (빨강)
		{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } }, // 1: 오른쪽 위 앞 (녹색)
		{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } }, // 2: 오른쪽 아래 앞 (파랑)
		{ { -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f } }, // 3: 왼쪽 아래 앞 (노랑)

		// 뒷면 정점들 (z = -0.5)
		{ { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f } }, // 4: 왼쪽 위 뒤 (자홍)
		{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f } }, // 5: 오른쪽 위 뒤 (청록)
		{ {  0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f, 1.0f } }, // 6: 오른쪽 아래 뒤 (회색)
		{ { -0.5f, -0.5f, -0.5f }, { 0.8f, 0.8f, 0.8f, 1.0f } }  // 7: 왼쪽 아래 뒤 (연회색)
};

// 플레이어 인덱스
std::vector<uint16_t> playerIndices = {
	// 앞면
	0, 1, 2,    0, 2, 3,
	// 뒷면
	5, 4, 7,    5, 7, 6,
	// 왼쪽면
	4, 0, 3,    4, 3, 7,
	// 오른쪽면
	1, 5, 6,    1, 6, 2,
	// 위면 (이게 실제 땅 표면!)
	4, 5, 1,    4, 1, 0,
	// 아래면
	3, 2, 6,    3, 6, 7
};