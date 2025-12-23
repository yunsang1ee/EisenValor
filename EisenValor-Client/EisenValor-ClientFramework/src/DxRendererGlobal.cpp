#include "stdafxClientFramework.h"
#include "DxRendererGlobal.h"
#include "IRenderPass.h"
#include "Scene.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxFeatureCaps.h"
#include "DxFrameResource.h"
#include "DxSwapChain.h"

void DxRendererGlobal::Initialize()
{
	auto& device = MANAGER(DxDeviceGlobal);
	m_featureCaps = DxFeatureCaps::Query(device.GetDevice(), device.GetAdapter());
	m_featureCaps.LogCapabilities();

	if (m_featureCaps.rayTracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
	{
		DEBUG_LOG_FMT("[GameFramework] ERROR: DXR not supported on this device!\n");
		return;
	}

	// RTV Descriptor Heap 持失
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = kFrameCount;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device.GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
	m_rtvDescriptorSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Frame Resources 持失
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

	auto& device = MANAGER(DxDeviceGlobal);
	auto& commandQueue = MANAGER(DxGfxCommandQueueGlobal);

	m_swapChain = std::make_unique<DxSwapChain>(
		device.GetDevice(), device.GetFactory(), commandQueue, hwnd, width, height, kFrameCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_rtvDescriptorSize
	);

	m_swapChain->SetResizeCallback([this](uint32_t w, uint32_t h) { OnResize(w, h); });

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

void DxRendererGlobal::Render(DxFrameResource* frame, Scene* scene, DxSwapChain* swapChain)
{
	if (!scene)
	{
		DEBUG_LOG_FMT("[DxRendererGlobal] WARNING: Scene is null!\n");
		return;
	}

	for (auto& entry : m_renderPasses)
	{
		entry.pass->Execute(frame, scene);
	}
}

void DxRendererGlobal::OnResize(uint32_t width, uint32_t height)
{
	for (auto& entry : m_renderPasses)
	{
		entry.pass->OnResize(width, height);
	}
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