#include "stdafxClientFramework.h"
#include "GameFramework.h"
#include "GlobalInterfaces.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "Vertex.h"

using namespace DirectX;


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
	const UINT vertexBufferSize = sizeof(Vertex) * cubeVertices.size();

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
	memcpy(pVertexDataBegin, cubeVertices.data(), vertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	// VertexBufferView 설정
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	// 인덱스 버퍼 크기 계산
	const UINT indexBufferSize = sizeof(uint16_t) * playerIndices.size();

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
	memcpy(pIndexDataBegin, playerIndices.data(), indexBufferSize);
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
	ThrowIfFailed(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

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

	// 상수버퍼 생성 25.07.20
	// 상수 버퍼용 힙 속성
	D3D12_HEAP_PROPERTIES cbHeapProps = {};
	cbHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 상수 버퍼 크기 (256바이트 정렬)
	const UINT constantBufferSize = (sizeof(ConstantBuffer) + 255) & ~255;

	// 상수 버퍼 리소스 설명
	D3D12_RESOURCE_DESC cbResourceDesc = {};
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = constantBufferSize;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 상수 버퍼 생성
	ThrowIfFailed(device.GetDevice()->CreateCommittedResource(
		&cbHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer)
	));

	// 상수 버퍼 매핑 CPU가 읽을 수 있게 함
	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
	
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
	return true;
}

void GameFramework::Run()
{
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

	// 플레이어 이동 처리 WASD
	float deltaTime = Globals::Timer().GetDeltaTime();
	float moveDistance = m_playerSpeed * deltaTime;

	if (Globals::Input().GetInput(87))  // W키
		m_playerZ += moveDistance;  // 앞으로
	if (Globals::Input().GetInput(83))  // S키
		m_playerZ -= moveDistance;  // 뒤로
	if (Globals::Input().GetInput(65))  // A키
		m_playerX -= moveDistance;  // 왼쪽으로
	if (Globals::Input().GetInput(68))  // D키
		m_playerX += moveDistance;  // 오른쪽으로
	if (Globals::Input().GetInput(72))  // 위로 (H)
		m_playerY += moveDistance;
	if (Globals::Input().GetInput(76))  // 아래로 (L)
		m_playerY -= moveDistance;

	// 위치 디버깅
	static float lastX = 0, lastY = 1, lastZ = 0;
	if (m_playerX != lastX || m_playerZ != lastZ) {
		DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n",
			m_playerX, m_playerY, m_playerZ);
		lastX = m_playerX; lastY = m_playerY; lastZ = m_playerZ;
	}

	// ===== 마우스로 카메라 이동 =====
	bool isLeftButtonPressed = Globals::Input().GetInput(VK_LBUTTON);
	// 현재 마우스 위치
	auto mousePos = Globals::Input().GetMousePosition();

	if (isLeftButtonPressed)
	{
		if (!m_isMouseDragging)
		{
			m_isMouseDragging = true;
			m_lastMouseX = mousePos.x;  // 시작 위치 저장
			m_lastMouseY = mousePos.y;
			DEBUG_LOG_FMT("Camera drag started at ({:.1f}, {:.1f})\n", mousePos.x, mousePos.y);
		}
		else
		{
			// 움직임 감지
			float deltaX = mousePos.x - m_lastMouseX;
			float deltaY = mousePos.y - m_lastMouseY;

			if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f) 
			{
				// 카메라 회전 업데이트
				m_cameraYaw += deltaX * m_mouseSensitivity;
				//m_cameraPitch += deltaY * m_mouseSensitivity;

				// Pitch 제한 (위아래 회전 제한)
				m_cameraPitch = std::clamp(m_cameraPitch, -1.5f, 1.5f);

				//디버깅
				DEBUG_LOG_FMT("Camera rotating - Delta({:.1f}, {:.1f}) Yaw: {:.2f}, Pitch: {:.2f}\n",
					deltaX, deltaY, m_cameraYaw, m_cameraPitch);
			}

			m_lastMouseX = mousePos.x;
			m_lastMouseY = mousePos.y;
		}
	}
	else
	{
		if (m_isMouseDragging)
		{
			// 드래그 종료
			m_isMouseDragging = false;
			DEBUG_LOG_FMT("Camera drag ended\n");
		}
	}

	//마우스 휠로 줌인아웃
	int wheelDelta = Globals::Input().GetWheelScroll();
	if (wheelDelta != 0)
	{
		m_cameraDistance -= wheelDelta * 0.001f;
		m_cameraDistance = std::clamp(m_cameraDistance, 5.0f, 30.0f);
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

	float camX = m_playerX - m_cameraDistance * sinf(m_cameraYaw) * cosf(m_cameraPitch);
	float camY = m_playerY + 3.0f + m_cameraDistance * sinf(m_cameraPitch);
	float camZ = m_playerZ - m_cameraDistance * cosf(m_cameraYaw) * cosf(m_cameraPitch);

	float lookX = m_playerX + 2.0f * sinf(m_cameraYaw); 
	float lookY = m_playerY + 1.0f + 2.0f * sinf(m_cameraPitch);
	float lookZ = m_playerZ + 2.0f * cosf(m_cameraYaw);

	XMMATRIX view = XMMatrixLookAtLH(
		XMVectorSet(camX, camY, camZ, 0.0f),          // 플레이어 뒤쪽 위치
		XMVectorSet(lookX, lookY, lookZ, 0.0f),       // 플레이어 주변을 바라봄
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)          // 업 벡터
	);

	// 투영 행렬
	XMMATRIX projection = XMMatrixPerspectiveFovLH(
		XM_PI / 4.0f,                                    // 45도 시야각
		(float)m_swapChain->GetWidth() / m_swapChain->GetHeight(), // 종횡비
		0.1f,                                           // 가까운 클리핑 평면
		100.0f                                          // 먼 클리핑 평면
	);

	// MVP 행렬 조합
	XMMATRIX mvp = world * view * projection;

	// 상수 버퍼에 복사
	XMStoreFloat4x4(&m_constantBufferData.mvp, XMMatrixTranspose(mvp)); // 전치 필요
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

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


	// 플레이어
	XMMATRIX cubeWorld = XMMatrixScaling(0.3f, 0.8f, 0.3f) * XMMatrixTranslation(m_playerX, m_playerY, m_playerZ);
	XMMATRIX cubeMVP = cubeWorld * view * projection;

	XMStoreFloat4x4(&m_constantBufferData.mvp, XMMatrixTranspose(cubeMVP));
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	context.CommandList()->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
	context.CommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);
	

	// 백버퍼를 프레젠트 상태로 전환
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	context.CommandList()->ResourceBarrier(1, &barrier);

	// 커맨드 실행
	m_commandContextPool->SignalCurrentFrame();
	// 화면에 표시
	m_swapChain->Present(1, 0);


}


