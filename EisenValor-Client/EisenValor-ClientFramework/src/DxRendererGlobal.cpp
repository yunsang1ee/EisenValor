#include "stdafxClientFramework.h"
#include "DxRendererGlobal.h"
#include "IRenderPass.h"
#include "Scene.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxFeatureCaps.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxFrameResource.h"
#include "DxSwapChain.h"

DxRendererGlobal::DxRendererGlobal() = default;

DxRendererGlobal::~DxRendererGlobal() = default;

void DxRendererGlobal::Initialize()
{
	auto& device = MANAGER(DxDeviceGlobal);
	m_featureCaps = DxFeatureCaps::Query(device.GetDevice(), device.GetAdapter());
	m_featureCaps.LogCapabilities();

	if (m_featureCaps.rayTracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
	{
		DEBUG_LOG_FMT("[GameFramework] ERROR: DXR not supported on this device!\n");
		m_isInitialized = false;
		return;
	}

	m_isInitialized = true;
	m_currentFrameIndex = 0;

	// RTV Descriptor Heap 생성
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = kFrameCount;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device.GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
	m_rtvDescriptorSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Frame Resources 생성
	for (uint32_t i = 0; i < kFrameCount; ++i)
	{
		m_frameResources[i] = std::make_unique<DxFrameResource>();
		m_frameResources[i]->Initialize(device.GetDevice(), i);
	}

	DEBUG_LOG_FMT("[DxRendererGlobal] Initialized (SwapChain not created yet - call CreateSwapChain)\n");
}

void DxRendererGlobal::CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height)
{
	if (m_swapChain)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] WARNING: SwapChain already exists!\n");
		return;
	}

	if (!m_rtvDescriptorHeap)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] ERROR: Renderer not initialized (RTV heap is null)\n");
		return;
	}

	auto& device = MANAGER(DxDeviceGlobal);
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);

	m_swapChain = std::make_unique<DxSwapChain>(
		device.GetDevice(), device.GetFactory(), commandQueue, hwnd, width, height, kFrameCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_rtvDescriptorSize
	);

	m_swapChain->SetResizeCallback(
		[this](uint32_t w, uint32_t h)
		{
			for (auto& entry : m_renderPasses)
			{
				entry.pass->OnResize(w, h);
			}
		}
	);

	DEBUG_LOG_FMT("[DxRendererGlobal] SwapChain created: {}x{}\n", width, height);
}

void DxRendererGlobal::Release()
{
	if (m_swapChain)
	{
		auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
		commandQueue.WaitForIdle();
	}

	ClearAllPasses();

	for (auto& frameRes : m_frameResources)
	{
		frameRes.reset();
	}

	m_swapChain.reset();
	m_rtvDescriptorHeap.Reset();

	DEBUG_LOG_FMT("[DxRendererGlobal] Released\n");
}

void DxRendererGlobal::AddRenderPass(const std::string& name, std::unique_ptr<IRenderPass> pass)
{
	for (const auto& entry : m_renderPasses)
	{
		if (entry.name == name)
		{
			DEBUG_LOG_FMT("[DxRendererGlobal] WARNING: Pass already exists: {}\n", name);
			return;
		}
	}

	pass->Initialize();
	m_renderPasses.push_back({name, std::move(pass)});

	DEBUG_LOG_FMT("[DxRendererGlobal] Pass added: {}\n", name);
}

void DxRendererGlobal::RemoveRenderPass(const std::string& name)
{
	auto iter = std::remove_if(
		m_renderPasses.begin(), m_renderPasses.end(),
		[&name](const RenderPassEntry& entry)
		{
			if (entry.name == name)
			{
				entry.pass->Release();
				return true;
			}
			return false;
		}
	);

	if (iter != m_renderPasses.end())
	{
		m_renderPasses.erase(iter, m_renderPasses.end());
		DEBUG_LOG_FMT("[DxRendererGlobal] Pass removed: {}\n", name);
	}
}

IRenderPass* DxRendererGlobal::GetRenderPass(const std::string& name) const
{
	for (const auto& entry : m_renderPasses)
	{
		if (entry.name == name)
		{
			return entry.pass.get();
		}
	}
	return nullptr;
}

void DxRendererGlobal::BeginFrame()
{
	if (!m_isInitialized || !m_swapChain)
	{
		DEBUG_LOG_FMT(
			"[DxRendererGlobal] ERROR: Not ready (init:{}, swapChain:{})\n", m_isInitialized, nullptr != m_swapChain 
		);
		return;
	}

	m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();
	auto* frame = m_frameResources[m_currentFrameIndex].get();
	frame->BeginFrame();
}

void DxRendererGlobal::Render(Scene* scene)
{
	if (!scene)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] WARNING: Scene is null!\n");
		return;
	}

	auto* frame = m_frameResources[m_currentFrameIndex].get();

	for (auto& entry : m_renderPasses)
	{
		entry.pass->Execute(frame, scene);
	}
}

void DxRendererGlobal::EndFrame()
{
	if (!m_swapChain)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] ERROR: SwapChain not initialized!\n");
		return;
	}

	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);
	auto* frame = m_frameResources[m_currentFrameIndex].get();

	frame->ExecuteAndSignal(commandQueue.GetQueue());
	const uint64_t signaledFence = commandQueue.SignalFence();

	m_swapChain->PresentMaxPerformance();
	frame->WaitForCompletion();

	auto& gc = MANAGER(DxGarbageCollectorGlobal);
	gc.SetCurrentFrameFence(FenceHandle{EQueueType::Graphics, signaledFence});
	gc.ProcessCompletedReleases(commandQueue.GetCompletedFenceValue());
}

void DxRendererGlobal::OnResize(uint32_t width, uint32_t height)
{
	if (!m_swapChain)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] ERROR: SwapChain not initialized!\n");
		return;
	}

	if (width == 0 || height == 0)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] Warning: Resize called with zero dimension: {}x{}\n", width, height);
		return;
	}

	auto& device = MANAGER(DxDeviceGlobal);
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);

	commandQueue.WaitForIdle();
		
	m_swapChain->OnResize(
		device.GetDevice(), width, height, m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		m_rtvDescriptorSize
	);	
	DEBUG_LOG_FMT("[DxRendererGlobal] Resize handled: {}x{}\n", width, height);
}

void DxRendererGlobal::ClearAllPasses()
{
	for (auto& entry : m_renderPasses)
	{
		entry.pass->Release();
	}
	m_renderPasses.clear();
	DEBUG_LOG_FMT("[DxRendererGlobal] All passes cleared\n");
}

DxFrameResource* DxRendererGlobal::GetCurrentFrame() const
{
	if (!m_isInitialized)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] ERROR: Not initialized!\n");
		return nullptr;
	}
	return m_frameResources[m_currentFrameIndex].get();
}
