#include "stdafxClient.h"
#include "UIRenderPass.h"
#include <DxCommandContext.h>
#include <DxUtils.h>
#include <DxRendererGlobal.h>
#include <DxDeviceGlobal.h>
#include <DxShaderCompilerGlobal.h>
#include "UI/UIGlobal.h" 
#include "UI/UITextureGlobal.h"

UIRenderPass::UIRenderPass() {}

UIRenderPass::~UIRenderPass() {}

void UIRenderPass::Initialize()
{
	auto* renderer = &GLOBAL(DxRendererGlobal); // 포인터로 접근
	auto* swapChain = renderer->GetSwapChain();

	if (swapChain)
	{
		m_screenWidth = swapChain->GetWidth();
		m_screenHeight = swapChain->GetHeight();
	}
	
	CreateUIPipelineState();
	CreateVertexBuffer();
	CreateInstanceBuffer();

	m_initialized = true;
	DEBUG_LOG_FMT("[UIRenderPass] Initialized ({}x{})\n", m_screenWidth, m_screenHeight);
}

void UIRenderPass::Release()
{
	m_rootSignature.Reset();
	m_pipelineState.Reset();
	m_vertexBuffer.reset();
	m_instanceBuffer.reset();
	m_initialized = false;
}

void UIRenderPass::Execute(DxFrameResource* frame, Scene* scene)
{
	if (!m_initialized || !m_pipelineState || !m_vertexBuffer)
	{
		return;
	}

	auto* context = frame->GetMainContext();
	auto* cmdList = context->CommandList();
	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();

	// 1. 리소스 배리어 설정
	auto* backBuffer = swapChain->GetCurrentBackBuffer();

	D3D12_RESOURCE_BARRIER barrier;

	// 백버퍼: PRESENT -> RENDER_TARGET
	barrier =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->ResourceBarrier(1, &barrier);

	// 2. 렌더 타겟 설정
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChain->GetCurrentBackBufferRTV();
	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// 3. 뷰포트 설정
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(m_screenWidth);
	viewport.Height = static_cast<float>(m_screenHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = m_screenWidth;
	scissorRect.bottom = m_screenHeight;
	cmdList->RSSetScissorRects(1, &scissorRect);

	// 4. 파이프라인 상태 설정
	cmdList->SetPipelineState(m_pipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	// 5. SRV Descriptor Table 설정
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);
	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);
	cmdList->SetGraphicsRootDescriptorTable(0, descHeap.GetGPUHandleStart());

	//  모든 UI 컴포넌트 인스턴싱 렌더링
	RenderAllUIInstanced(frame);

	// 8. 백버퍼
	barrier =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	cmdList->ResourceBarrier(1, &barrier);

	//DEBUG_LOG_FMT("[UIRenderPass] Rendered all UI components\n");
}

void UIRenderPass::OnResize(uint32_t width, uint32_t height)
{
	m_screenWidth = width;
	m_screenHeight = height;
}

void UIRenderPass::CreateUIPipelineState()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& shaderCompiler = GLOBAL(DxShaderCompilerGlobal);

	// 1. 셰이더 컴파일
	auto vsBlob = shaderCompiler.CompileShaderFromFile(
		L"UIVertexShader", L"Resource/Shader/UIVertexShader.hlsl", "main", "vs_6_6"
	);

	auto psBlob =
		shaderCompiler.CompileShaderFromFile(L"UIPixelShader", L"Resource/Shader/UIPixelShader.hlsl", "main", "ps_6_6");

	// 2. 루트 시그니처 생성
	D3D12_ROOT_PARAMETER rootParams[1] = {};

	// Param 0: Texture Descriptor Table (t0~tN)
	D3D12_DESCRIPTOR_RANGE textureRange = {};
	textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	textureRange.NumDescriptors = 1000;
	textureRange.BaseShaderRegister = 0; // t0부터
	textureRange.RegisterSpace = 0;
	textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &textureRange;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Static Sampler 정의 (s0)
	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.MipLODBias = 0.0f;
	staticSampler.MaxAnisotropy = 1;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSampler.MinLOD = 0.0f;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0; // s0
	staticSampler.RegisterSpace = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1; // Static Sampler 
	rootSigDesc.pStaticSamplers = &staticSampler;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT			 hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	
	if (FAILED(hr))
	{
		if (error)
		{
			DEBUG_LOG_FMT("[UIRenderPass] Root signature error: {}\n", (char*)error->GetBufferPointer());
		}
		return;
	}

	ThrowIfFailed(device.GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)
	));

	DEBUG_LOG_FMT("[UIRenderPass] Root signature created\n");

	// 3. 입력 레이아웃 정의 (정점 + 인스턴스)
	D3D12_INPUT_ELEMENT_DESC inputElements[] = {
		// 정점 데이터 (슬롯 0, PER_VERTEX_DATA)
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// 인스턴스 데이터 (슬롯 1, PER_INSTANCE_DATA)
		{"TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TEXTURE_INDEX", 0, DXGI_FORMAT_R32_UINT, 1, 80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
	};

	// 4. 파이프라인 상태 생성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = {inputElements, _countof(inputElements)};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};

	// 래스터라이저 설정
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // UI는 컬링 안함
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = 0;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// 블렌딩 설정
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// 깊이 스텐실 설정
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	// 기타 설정
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 백버퍼 포맷
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(device.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	DEBUG_LOG_FMT("[UIRenderPass] Pipeline state created\n");
}

void UIRenderPass::CreateVertexBuffer()
{
	auto& device = GLOBAL(DxDeviceGlobal);

	// UI 쿼드용 정점 데이터
	std::vector<UIVertex> quadVertices = {
		// 첫 번째 삼각형
		{{0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 좌상단 - 빨간색
		{{1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // 우상단 - 초록색
		{{0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}, // 좌하단 - 파란색

		// 두 번째 삼각형
		{{1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // 우상단 - 초록색
		{{1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}}, // 우하단 - 노란색
		{{0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}  // 좌하단 - 파란색
	};

	const uint64_t vertexBufferSize = quadVertices.size() * sizeof(UIVertex);

	// DxBuffer로 정점 버퍼 생성 (GPU 전용)
	m_vertexBuffer = std::make_unique<DxBuffer>();
	m_vertexBuffer->Initialize(
		device.GetDevice(), vertexBufferSize, EBufferUsage::Vertex, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COMMON, "UIVertexBuffer" // 상태 변경
	);

	DEBUG_LOG_FMT(
		"[UIRenderPass] Vertex buffer created ({} bytes, {} vertices)\n", vertexBufferSize, quadVertices.size()
	);

	// 데이터 업로드는 Execute()에서 DxUploadHeap 사용
	// 지금은 버퍼만 생성
}

DirectX::XMMATRIX UIRenderPass::CalculateUITransform(float x, float y, float width, float height)
{
	// 화면 좌표를 NDC로 변환하는 Transform 행렬 계산
	// 1단계: NDC 크기 계산
	float ndcWidth = (width / m_screenWidth) * 2.0f;
	float ndcHeight = (height / m_screenHeight) * 2.0f;

	// 2단계: NDC 위치 계산 (왼쪽 상단 모서리)
	float ndcX = (x / m_screenWidth) * 2.0f - 1.0f;
	float ndcY = 1.0f - (y / m_screenHeight) * 2.0f;

	// 3단계: 행렬 조합
	// 스케일링: (0~1) -> (ndcWidth, ndcHeight)
	DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(ndcWidth, -ndcHeight, 1.0f); // Y축 뒤집기

	// 이동: 원점에서 (ndcX, ndcY)로
	DirectX::XMMATRIX translateMatrix = DirectX::XMMatrixTranslation(ndcX, ndcY, 0.0f);

	// 최종 변환: 이동 * 스케일링
	DirectX::XMMATRIX transform = scaleMatrix * translateMatrix;

	return transform;
}

void UIRenderPass::CreateInstanceBuffer()
{
	auto& device = GLOBAL(DxDeviceGlobal);

	// 인스턴스 버퍼 크기 계산(최대 1024개)
	const uint64_t instanceBufferSize = m_maxInstances * sizeof(UIInstanceData);

	// 인스턴스 버퍼 생성 (동적 업로드용, 초기 상태 COPY_DEST)
	m_instanceBuffer = std::make_unique<DxBuffer>();
	m_instanceBuffer->Initialize(
		device.GetDevice(), instanceBufferSize, EBufferUsage::Vertex, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COMMON, "UIInstanceBuffer"
	);

	DEBUG_LOG_FMT(
		"[UIRenderPass] Instance buffer created ({} bytes, max {} instances)\n", instanceBufferSize, m_maxInstances
	);
}

void UIRenderPass::RenderAllUIInstanced(DxFrameResource* frame)
{
	// 0. 첫 프레임에만 정점 데이터 업로드
	if (!m_vertexDataUploaded)
	{
		auto* cmdList = frame->GetMainContext()->CommandList();
		auto* uploadHeap = frame->GetUploadHeap();

		std::vector<UIVertex> quadVertices = {
			{{0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, {{1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
			{{0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, {{1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
			{{1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, {{0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
		};

		auto allocation = uploadHeap->UploadVertexBuffer(quadVertices);

		cmdList->CopyBufferRegion(
			m_vertexBuffer->GetResource(), 0, uploadHeap->GetResource(), allocation.offset, allocation.size
		);

		D3D12_RESOURCE_BARRIER barrier = DxUtils::CreateTransitionBarrier(
			m_vertexBuffer->GetResource(),
			D3D12_RESOURCE_STATE_COPY_DEST, // CopyBufferRegion 후의 실제 상태
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);
		cmdList->ResourceBarrier(1, &barrier);

		m_vertexDataUploaded = true;
	}

	auto&		uiGlobal = GLOBAL(UIGlobal);
	const auto& components = uiGlobal.GetUIComponents();

	if (components.empty())
		return;

	// 1. 유효한 UI 컴포넌트 개수 확인
	uint32_t instanceCount = 0;
	for (const auto& ui : components)
	{
		if (ui && ui.get())
			instanceCount++;
	}

	if (instanceCount == 0)
		return;

	if (instanceCount > m_maxInstances)
	{
		DEBUG_LOG_FMT(
			"[UIRenderPass] Warning: Too many UI instances ({} > {}), truncating\n", instanceCount, m_maxInstances
		);
		instanceCount = m_maxInstances;
	}

	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* uploadHeap = frame->GetUploadHeap();

	// 2. 인스턴스 데이터 준비
	std::vector<UIInstanceData> instanceData;
	instanceData.reserve(instanceCount);

	for (const auto& ui : components)
	{
		if (!ui || instanceData.size() >= instanceCount)
			break;

		auto	 pos = ui->GetPosition();
		auto	 size = ui->GetSize();
		auto	 color = ui->GetColor();
		uint32_t textureId = ui->GetTextureId();

		// Transform 계산
		DirectX::XMMATRIX transform = CalculateUITransform(pos.x, pos.y, size.x, size.y);
		DirectX::XMMATRIX transposedTransform = DirectX::XMMatrixTranspose(transform);

		UIInstanceData instance;
		DirectX::XMStoreFloat4x4(&instance.transform, transposedTransform);
		instance.color = color;

		// 텍스처 ID를 SRV 인덱스로 변환
		if (textureId > 0)
		{
			auto&	 textureGlobal = GLOBAL(UITextureGlobal);
			uint32_t srvIndex = textureGlobal.GetSRVIndex(textureId);
			instance.textureIndex = srvIndex;
		}
		else
		{
			instance.textureIndex = 0; // 텍스처 없음
		}
		instance.padding[0] = instance.padding[1] = instance.padding[2] = 0;

		instanceData.push_back(instance);
	}

	// 3. 인스턴스 데이터 업로드
	const uint64_t instanceDataSize = instanceCount * sizeof(UIInstanceData);
	auto		   allocation = uploadHeap->UploadRawData(instanceData.data(), instanceDataSize, 16);

	cmdList->CopyBufferRegion(
		m_instanceBuffer->GetResource(), 0, uploadHeap->GetResource(), allocation.offset, allocation.size
	);

	D3D12_RESOURCE_BARRIER barrier = DxUtils::CreateTransitionBarrier(
		m_instanceBuffer->GetResource(),
		D3D12_RESOURCE_STATE_COPY_DEST, // CopyBufferRegion 후의 실제 상태
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
	);
	cmdList->ResourceBarrier(1, &barrier);

	// 5. 정점 버퍼와 인스턴스 버퍼 설정
	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {};

	// 슬롯 0: 정점 버퍼 (쿼드 정점 데이터)
	vbvs[0] = m_vertexBuffer->GetVertexBufferView(sizeof(UIVertex));

	// 슬롯 1: 인스턴스 버퍼
	vbvs[1] = m_instanceBuffer->GetVertexBufferView(sizeof(UIInstanceData));

	cmdList->IASetVertexBuffers(0, 2, vbvs);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 6. 드로우 콜
	cmdList->DrawInstanced(6, instanceCount, 0, 0); // 6개 정점, instanceCount개 인스턴스

	//DEBUG_LOG_FMT("[UIRenderPass] Rendered {} UI instances in one draw call\n", instanceCount);
}