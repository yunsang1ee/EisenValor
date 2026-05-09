#include "stdafxClient.h"
#include "UIRenderPass.h"
#include <DxCommandContext.h>
#include <PixProfiler.h>
#include <DxUploadHeap.h>
#include <DxUtils.h>
#include <DxRendererGlobal.h>
#include <DxDeviceGlobal.h>
#include <DxShaderCompilerGlobal.h>
#include "UIGlobal.h"
#include "Scene.h"
#include "UIComponent.h"
#include "DenseList.h"
#include "ComponentStorage.h"
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "TextureResource.h"
#include "DxTexture.h"
#include <algorithm>

UIRenderPass::UIRenderPass() {}

UIRenderPass::~UIRenderPass() {}

void UIRenderPass::Initialize()
{
	auto* renderer = &GLOBAL(DxRendererGlobal);
	auto* swapChain = renderer->GetSwapChain();

	if (swapChain)
	{
		m_screenWidth = swapChain->GetWidth();
		m_screenHeight = swapChain->GetHeight();
	}

	// UI 전용 Root Signature PSO 생성
	CreateUIPipelineState();
	CreateVertexBuffer();
	CreateInstanceBuffer();

	m_initialized = true;
	//	DEBUG_LOG_FMT("[UIRenderPass] Initialized ({}x{})", m_screenWidth, m_screenHeight);
}

void UIRenderPass::Release()
{
	m_rootSignature.Reset();
	m_pipelineState.Reset();
	m_vertexBuffer.reset();
	m_instanceBuffer.reset();
	m_initialized = false;
}

void UIRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"UIRenderPass.Execute");

	if (!m_initialized || !m_pipelineState || !m_vertexBuffer || !scene || !renderContext)
	{
		return;
	}

	auto& context = *frame->GetMainContext();
	auto* cmdList = context.CommandList();
	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();

	// 1. 백버퍼 상태 전환 (Present -> RenderTarget)
	auto*				   backBuffer = swapChain->GetCurrentBackBuffer();
	D3D12_RESOURCE_BARRIER barrier =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->ResourceBarrier(1, &barrier);

	// 2. 렌더 타겟 및 뷰포트 설정 (BackBuffer 렌더링)
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChain->GetCurrentBackBufferRTV();
	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	D3D12_VIEWPORT viewport = {0.0f, 0.0f, (float)m_screenWidth, (float)m_screenHeight, 0.0f, 1.0f};
	cmdList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect = {0, 0, (LONG)m_screenWidth, (LONG)m_screenHeight};
	cmdList->RSSetScissorRects(1, &scissorRect);

	// 3. 파이프라인 상태 설정
	cmdList->SetPipelineState(m_pipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	// Descriptor Heap 설정 (Bindless Texture)
	auto&				  descHeap = GLOBAL(DxDescriptorHeapGlobal);
	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);
	cmdList->SetGraphicsRootDescriptorTable(0, descHeap.GetGPUHandleStart());

	// 4. UI 컴포넌트 수집 및 인스턴싱 렌더링 실행
	RenderAllUIInstanced(frame, scene);

	// 5. 백버퍼 상태 복구 (RenderTarget -> Present)
	barrier =
		DxUtils::CreateTransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	cmdList->ResourceBarrier(1, &barrier);
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

	// 셰이더 컴파일 (VS, PS)
	auto vsBlob = shaderCompiler.CompileShaderFromFile(
		L"UIVertexShader", L"Resource/Shader/UIVertexShader.hlsl", "main", "vs_6_6"
	);
	auto psBlob =
		shaderCompiler.CompileShaderFromFile(L"UIPixelShader", L"Resource/Shader/UIPixelShader.hlsl", "main", "ps_6_6");

	// 루트 시그니처 구성
	D3D12_ROOT_PARAMETER rootParams[1] = {};

	// Param 0: 텍스처 테이블 (Bindless)
	D3D12_DESCRIPTOR_RANGE textureRange = {};
	textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	textureRange.NumDescriptors = GLOBAL(DxDescriptorHeapGlobal).GetCapacity();
	textureRange.BaseShaderRegister = 0;
	textureRange.RegisterSpace = 0;
	textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &textureRange;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Static Sampler (UI용 Point/Linear)
	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.ShaderRegister = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &staticSampler;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature, error;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	device.GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)
	);

	// 입력 레이아웃 (Instance Data에 UV 추가됨)
	D3D12_INPUT_ELEMENT_DESC inputElements[] = {
		// Vertex Data
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// Instance Data
		{"TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		// UV Min/Max 추가
		{"UV_MIN", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"UV_MAX", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 88, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TEXTURE_INDEX", 0, DXGI_FORMAT_R32_UINT, 1, 96, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"PADDING", 0, DXGI_FORMAT_R32G32B32_UINT, 1, 100, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"PADDING", 1, DXGI_FORMAT_R32G32B32A32_UINT, 1, 112, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = {inputElements, _countof(inputElements)};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};

	// 명세서: Depth Disable, Alpha Blend Enable
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;

	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.DepthStencilState.DepthEnable = FALSE; // Depth Test 끔 (Z-Order로 정렬해서 그림)
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	device.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
}

void UIRenderPass::CreateVertexBuffer()
{
	auto&				  device = GLOBAL(DxDeviceGlobal);
	std::vector<UIVertex> quadVertices = {
		{{0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, {{1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		{{0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, {{1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		{{1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, {{0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
	};
	m_vertexBuffer = std::make_unique<DxBuffer>();
	m_vertexBuffer->Initialize(
		device.GetDevice(), (uint32_t)(quadVertices.size() * sizeof(UIVertex)), EBufferUsage::Vertex,
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, "UIVertexBuffer"
	);
}

void UIRenderPass::CreateInstanceBuffer()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	m_instanceBuffer = std::make_unique<DxBuffer>();
	m_instanceBuffer->Initialize(
		device.GetDevice(), m_maxInstances * sizeof(UIInstanceData), EBufferUsage::Vertex, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COMMON, "UIInstanceBuffer"
	);
}

DirectX::XMMATRIX UIRenderPass::CalculateUITransform(float x, float y, float width, float height)
{
	float ndcWidth = (width / m_screenWidth) * 2.0f;
	float ndcHeight = (height / m_screenHeight) * 2.0f;
	float ndcX = (x / m_screenWidth) * 2.0f - 1.0f;
	float ndcY = 1.0f - (y / m_screenHeight) * 2.0f;
	return DirectX::XMMatrixScaling(ndcWidth, -ndcHeight, 1.0f) * DirectX::XMMatrixTranslation(ndcX, ndcY, 0.0f);
}

void UIRenderPass::RenderAllUIInstanced(DxFrameResource* frame, Scene* scene)
{
	if (!m_vertexDataUploaded)
	{
		PixScopedCpuEvent uploadStaticEvent(L"UI.UploadStaticVertexData");

		auto*				  cmdList = frame->GetMainContext()->CommandList();
		auto*				  uploadHeap = frame->GetUploadHeap();
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
			m_vertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);
		cmdList->ResourceBarrier(1, &barrier);
		m_vertexDataUploaded = true;
	}

	if (!scene)
		return;

	std::vector<IUIComponent*> renderableUIs;

	{
		PixScopedCpuEvent collectEvent(L"UI.CollectComponents");

		// 1. ImageUI 수집
		ComponentStorage<ImageUIComponent>* imgStorage = scene->GetStorage<ImageUIComponent>();
		if (imgStorage)
		{
			for (ImageUIComponent& ui : imgStorage->GetList())
			{
				if (ui.GetGameObject()->IsActiveInHierarchy())
				{
					renderableUIs.push_back(&ui);
				}
			}
		}

		// 2. ButtonUI 수집
		ComponentStorage<ButtonUIComponent>* btnStorage = scene->GetStorage<ButtonUIComponent>();
		if (btnStorage)
		{
			for (ButtonUIComponent& ui : btnStorage->GetList())
			{
				if (ui.GetGameObject()->IsActiveInHierarchy())
				{
					renderableUIs.push_back(&ui);
				}
			}
		}
	}

	{
		PixScopedCpuEvent sortEvent(L"UI.Sort");

		// 2. Z-Order 기준 정렬
		std::stable_sort(
			renderableUIs.begin(), renderableUIs.end(),
			[](IUIComponent* a, IUIComponent* b) { return a->GetOrder() < b->GetOrder(); }
		);
	}

	// 3. 배칭 최적화 및 데이터 수집
	std::vector<UIInstanceData> instanceBufferData;
	instanceBufferData.reserve(m_maxInstances);

	std::vector<UIRenderData> componentRenderData;

	{
		PixScopedCpuEvent buildEvent(L"UI.BuildInstances");

		for (auto* ui : renderableUIs)
		{
			componentRenderData.clear();
			ui->GetRenderData(componentRenderData);

			for (const auto& rData : componentRenderData)
			{
				if (instanceBufferData.size() >= m_maxInstances)
					break;

				UIInstanceData inst = {}; // 초기화

				// 화면 좌표 -> NDC 변환
				auto transform = CalculateUITransform(rData.rect.x, rData.rect.y, rData.rect.width, rData.rect.height);
				DirectX::XMStoreFloat4x4(&inst.transform, DirectX::XMMatrixTranspose(transform));

				inst.color = rData.color;
				inst.uvMin = rData.uvMin;
				inst.uvMax = rData.uvMax;

				// 텍스처 바인딩 (Lazy-Resolving)
				if (rData.textureResource && rData.textureResource->IsReady() && rData.textureResource->GetTexture())
				{
					inst.textureIndex = rData.textureResource->GetTexture()->GetSRVIndex();
				}
				else
				{
					inst.textureIndex = 0;
				}

				instanceBufferData.push_back(inst);
			}
		}
	}

	if (instanceBufferData.empty())
		return;

	// 4. GPU에 데이터 업로드 및 드로우 콜 (직접 업로드 힙 참조)
	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* uploadHeap = frame->GetUploadHeap();

	DxUploadHeap::Allocation allocation;
	{
		PixScopedCpuEvent uploadEvent(L"UI.UploadInstances");
		allocation =
			uploadHeap->UploadRawData(instanceBufferData.data(), instanceBufferData.size() * sizeof(UIInstanceData), 16);
	}

	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
		m_vertexBuffer->GetVertexBufferView(sizeof(UIVertex)),
		{uploadHeap->GetResource()->GetGPUVirtualAddress() + allocation.offset, (UINT)allocation.size,
		 sizeof(UIInstanceData)}
	};
	cmdList->IASetVertexBuffers(0, 2, vbvs);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 1 Draw Call
	{
		PixScopedCpuEvent drawCpuEvent(L"UI.Draw");
		PixScopedCommandListEvent drawGpuEvent(cmdList, L"UI.Draw");
		cmdList->DrawInstanced(6, (UINT)instanceBufferData.size(), 0, 0);
	}
}
