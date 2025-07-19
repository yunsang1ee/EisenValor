#include "stdafxClientFramework.h"
#include "GameFramework.h"
#include "GlobalInterfaces.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "Vertex.h"


bool GameFramework::Initialize(HINSTANCE hInstance, HWND hwnd)
{
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
		commandQueue.GetQueue(),
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


	// 2. 삼각형 정점 데이터 25.07.20
	std::vector<Vertex> triangleVertices = {
		{ {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 빨간색 위쪽
		{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }, // 녹색 오른쪽 아래
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }  // 파란색 왼쪽 아래
	};

	// 정점 버퍼 생성
	const UINT vertexBufferSize = sizeof(Vertex) * triangleVertices.size();

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
	memcpy(pVertexDataBegin, triangleVertices.data(), vertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	// VertexBufferView 설정
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	
	// 3. 루트 시그니처 생성 (Simple)
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 0;
	rootSignatureDesc.pParameters = nullptr;
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
	ThrowIfFailed(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

	// 5. 입력 레이아웃 정의
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// PSO 생성하기
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

	return true;
}

void GameFramework::Run()
{
	Globals::Input().BeforeUpdate();

	Globals::Timer().Update();
	if(Globals::Timer().ShouldFixedUpdate())
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
		DEBUG_LOG_FMT("close");
		::PostQuitMessage(0);
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

	// 커맨드 리스트 시작
	context.Reset();

	// 현재 백버퍼 가져오기
	auto rtvHandle = m_swapChain->GetCurrentBackBufferRTV();
	auto backBuffer = m_swapChain->GetCurrentBackBuffer();

	// 백버퍼를 렌더 타겟으로 전환
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

	// 삼각형 그리기
	context.CommandList()->DrawInstanced(3, 1, 0, 0);

	// 백버퍼를 프레젠트 상태로 전환
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	context.CommandList()->ResourceBarrier(1, &barrier);

	// 커맨드 실행
	m_commandContextPool->SignalCurrentFrame();

	// 화면에 표시
	m_swapChain->Present(1, 0);


}

