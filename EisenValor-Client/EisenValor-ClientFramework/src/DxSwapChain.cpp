#include "stdafxClientFramework.h"
#include "DxSwapChain.h"
#include "DxUtils.h"
#include <DxCommandQueueGlobal.h>

DxSwapChain::DxSwapChain(
	ID3D12Device*				   device,
	IDXGIFactory6*				   factory,
	IDxGraphicsCommandQueueGlobal& commandQueue,
	HWND						   hwnd,
	uint32_t					   width,
	uint32_t					   height,
	uint32_t					   backBufferCount,
	DXGI_FORMAT					   format,
	D3D12_CPU_DESCRIPTOR_HANDLE	   rtvDescriptorStart,
	uint32_t					   rtvDescriptorSize
)
	: m_device(device), m_factory(factory), m_graphicsCommandQueueGlobal(commandQueue), m_hwnd(hwnd), m_width(width),
	  m_height(height), m_backBufferCount(backBufferCount), m_format(format), m_rtvDescriptorStart(rtvDescriptorStart),
	  m_rtvDescriptorSize(rtvDescriptorSize)
{
	assert(device && "[DxSwapChain] ID3D12Device is null.");
	assert(factory && "[DxSwapChain] IDXGIFactory6 is null.");
	assert(hwnd && "[DxSwapChain] HWND is null.");
	assert(width > 0 && "[DxSwapChain] Width > 0.");
	assert(height > 0 && "[DxSwapChain] Height > 0.");
	assert(backBufferCount > 0 && "[DxSwapChain] BackBufferCount > 0.");
	assert(rtvDescriptorStart.ptr && "[DxSwapChain] RTV Descriptor Start handle is null.");
	assert(rtvDescriptorSize > 0 && "[DxSwapChain] RTV Descriptor Size > 0.");

	BOOL tearingSupport = FALSE;
	if (SUCCEEDED(
			m_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupport, sizeof(tearingSupport))
		))
	{
		m_supportsTearing = (tearingSupport == TRUE);
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
		.Width = m_width,
		.Height = m_height,
		.Format = m_format,
		.Stereo = FALSE,
		.SampleDesc{.Count = 1, .Quality = 0},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = m_backBufferCount,
		.Scaling = DXGI_SCALING_NONE,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags = m_swapChainFlags = m_supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
	};

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(m_factory->CreateSwapChainForHwnd(
		m_graphicsCommandQueueGlobal.GetQueue(), m_hwnd, &swapChainDesc,
		nullptr, // Fullscreen Optimizations
		nullptr, &swapChain1
	));

	ThrowIfFailed(swapChain1.As(&m_swapChain));
	ThrowIfFailed(m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER)
	); // TODO: Alt+Enter Input Processing in InputGlobal

	CreateResources(device, m_graphicsCommandQueueGlobal.GetQueue(), m_rtvDescriptorStart, m_rtvDescriptorSize);

	EnumerateDisplayModes();
	m_currentDisplayMode = GetNativeDisplayMode();

	LogMonitorInfo(); // 디버깅 정보 출력

	DEBUG_LOG_FMT(
		"[DxSwapChain] SwapChain created: {}x{}, BackBuffers: {}, Format: {}, Tearing: {}\n", m_width, m_height,
		m_backBufferCount, (int)m_format, m_supportsTearing
	);
}

DxSwapChain::~DxSwapChain()
{
	ReleaseBackBuffers();
	DEBUG_LOG_FMT("[DxSwapChain] DxSwapChain destroyed.\n");
}

void DxSwapChain::ReleaseBackBuffers()
{
	m_graphicsCommandQueueGlobal.WaitForIdle(); // Ensure all GPU operations are complete before releasing resources
	for (auto& backBuffer : m_backBuffers)
	{
		if (backBuffer)
		{
			backBuffer.Reset();
		}
	}
	m_backBuffers.clear();
}

void DxSwapChain::Present(UINT syncInterval, UINT flags)
{
	UINT presentFlags = flags;
	if (m_supportsTearing && !m_isFullscreen && !m_isBorderlessFullscreen)
	{
		presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	++m_frameCount;
}

void DxSwapChain::PresentMaxPerformance()
{
	UINT presentFlags = 0;
	if (m_supportsTearing && !m_isFullscreen && !m_isBorderlessFullscreen)
	{
		presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	ThrowIfFailed(m_swapChain->Present(0, presentFlags));
	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	++m_frameCount;

	if (m_frameCount % 1000 == 0)
	{
		DEBUG_LOG_FMT(
			"[DxSwapChain] Performance Mode - Frame: {}, Tearing: {}, Windowed: {}\n", m_frameCount, m_supportsTearing,
			!m_isFullscreen && !m_isBorderlessFullscreen
		);
	}
}

void DxSwapChain::OnResize(
	ID3D12Device*				device,
	uint32_t					newWidth,
	uint32_t					newHeight,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart,
	uint32_t					rtvDescriptorSize
)
{
	if (newWidth == 0 || newHeight == 0)
	{
		DEBUG_LOG_FMT("[DxSwapChain] OnResize called with 0 width/height. Skipping resize.\n");
		return;
	}

	ReleaseBackBuffers();

	ThrowIfFailed(m_swapChain->ResizeBuffers(m_backBufferCount, newWidth, newHeight, m_format, m_swapChainFlags));

	m_rtvDescriptorStart = rtvDescriptorStart;
	m_rtvDescriptorSize = rtvDescriptorSize;
	m_width = newWidth;
	m_height = newHeight;

	CreateResources(device, m_graphicsCommandQueueGlobal.GetQueue(), m_rtvDescriptorStart, m_rtvDescriptorSize);

	DEBUG_LOG_FMT("[DxSwapChain] SwapChain resized to {}x{}. BackBuffers recreated.\n", m_width, m_height);
}

void DxSwapChain::CreateResources(
	ID3D12Device*				device,
	ID3D12CommandQueue*			commandQueue,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart,
	uint32_t					rtvDescriptorSize
)
{
	m_backBuffers.resize(m_backBufferCount);

	for (uint32_t i = 0; i < m_backBufferCount; ++i)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));

		auto rtvHandle = DxUtils::OffsetHandle(rtvDescriptorStart, i, rtvDescriptorSize);
		device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);

		std::wstring bufferName = L"SwapChainBackBuffer_" + std::to_wstring(i);
		DxUtils::SetDebugName(m_backBuffers[i].Get(), bufferName);
	}

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

ID3D12Resource* DxSwapChain::GetCurrentBackBuffer() const
{
	return m_backBuffers[m_currentBackBufferIndex].Get();
}

ID3D12Resource* DxSwapChain::GetBackBuffer(uint32_t index) const
{
	if (index >= m_backBufferCount)
	{
		DEBUG_LOG_FMT(
			"[DxSwapChain] Warning: Invalid back buffer index {}. Max is {}.\n", index, m_backBufferCount - 1
		);
		return nullptr;
	}
	if (!m_backBuffers[index])
	{
		DEBUG_LOG_FMT("[DxSwapChain] Error: Back buffer {} is null\n", index);
		return nullptr;
	}
	return m_backBuffers[index].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DxSwapChain::GetCurrentBackBufferRTV() const
{
	return GetBackBufferRTV(m_currentBackBufferIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE DxSwapChain::GetBackBufferRTV(uint32_t index) const
{
	if (index >= m_backBufferCount)
	{
		DEBUG_LOG_FMT("[DxSwapChain] Warning: Invalid RTV index {}. Max is {}.\n", index, m_backBufferCount - 1);
		return {};
	}

	return DxUtils::OffsetHandle(m_rtvDescriptorStart, index, m_rtvDescriptorSize);
}

DxSwapChain::SwapChainInfo DxSwapChain::GetInfo() const
{
	return SwapChainInfo{
		.width = m_width,
		.height = m_height,
		.backBufferCount = m_backBufferCount,
		.format = m_format,
		.currentBackBufferIndex = m_currentBackBufferIndex,
		.isFullscreen = m_isFullscreen,
		.isBorderlessFullscreen = m_isBorderlessFullscreen,
		.supportsTearing = m_supportsTearing,
		.frameCount = m_frameCount,
		.refreshRate = m_currentDisplayMode.refreshRateNumerator / m_currentDisplayMode.refreshRateDenominator,
	};
}

void DxSwapChain::SetFullscreen(bool enable)
{
	if (m_isBorderlessFullscreen || m_isFullscreen == enable)
		return;

	m_isFullscreen = enable;

	ThrowIfFailed(m_swapChain->SetFullscreenState(enable ? TRUE : FALSE, nullptr));

	if (enable)
	{
		OnResize(
			m_device, m_currentDisplayMode.width, m_currentDisplayMode.height, m_rtvDescriptorStart, m_rtvDescriptorSize
		);
	}
	else
	{
		RECT clientRect;
		GetClientRect(m_hwnd, &clientRect);
		uint32_t width = std::max(1u, static_cast<uint32_t>(clientRect.right - clientRect.left));
		uint32_t height = std::max(1u, static_cast<uint32_t>(clientRect.bottom - clientRect.top));

		OnResize(m_device, width, height, m_rtvDescriptorStart, m_rtvDescriptorSize);
	}

	DEBUG_LOG_FMT("[DxSwapChain] Fullscreen: {}\n", enable);
}

void DxSwapChain::SetBorderlessFullscreen(bool enable)
{
	if (m_isFullscreen || m_isBorderlessFullscreen == enable)
		return;

	if (enable)
	{
		StoreWindowedState();

		SetWindowLongPtr(m_hwnd, GWL_STYLE, WS_POPUP);
		SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		SetWindowPos(
			m_hwnd, HWND_TOPMOST, 0, 0, m_currentDisplayMode.width, m_currentDisplayMode.height,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW
		);

		m_isBorderlessFullscreen = true;

		OnResize(
			m_device, m_currentDisplayMode.width, m_currentDisplayMode.height, m_rtvDescriptorStart, m_rtvDescriptorSize
		);

		DEBUG_LOG_FMT(
			"[DxSwapChain] Borderless Fullscreen enabled: {}x{}\n", m_currentDisplayMode.width,
			m_currentDisplayMode.height
		);
	}
	else
	{
		m_isBorderlessFullscreen = false;

		RestoreWindowedState();
		DEBUG_LOG_FMT("[DxSwapChain] Borderless Fullscreen disabled\n");
	}
}

void DxSwapChain::EnumerateDisplayModes()
{
	m_supportedModes.clear();

	ComPtr<IDXGIOutput> output;
	if (SUCCEEDED(m_swapChain->GetContainingOutput(&output)))
	{
		UINT numModes = 0;
		output->GetDisplayModeList(m_format, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr);

		DEBUG_LOG_FMT("[DxSwapChain] Found {} display modes for format {}\n", numModes, (int)m_format);

		if (numModes > 0)
		{
			std::vector<DXGI_MODE_DESC> modes(numModes);
			output->GetDisplayModeList(m_format, DXGI_ENUM_MODES_INTERLACED, &numModes, modes.data());

			DEBUG_LOG_FMT("[DxSwapChain] Available display modes:\n");
			uint32_t logCount = std::min(numModes, 10u);
			for (uint32_t i = 0; i < logCount; ++i)
			{
				const auto& mode = modes[i];
				uint32_t	refreshRate = mode.RefreshRate.Numerator / mode.RefreshRate.Denominator;
				DEBUG_LOG_FMT("[DxSwapChain] Mode {}: {}x{} @ {}Hz\n", i, mode.Width, mode.Height, refreshRate);
			}

			for (const auto& mode : modes)
			{
				m_supportedModes.emplace_back(
					mode.Width, mode.Height, mode.RefreshRate.Numerator, mode.RefreshRate.Denominator, mode.Format,
					mode.Scaling
				);
			}
		}
	}

	DEBUG_LOG_FMT("[DxSwapChain] Enumerated {} display modes\n", m_supportedModes.size());
}

DxSwapChain::DisplayModeInfo DxSwapChain::GetNativeDisplayMode() const
{
	MONITORINFO primaryMi = {sizeof(primaryMi)};
	HMONITOR	primaryMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
	GetMonitorInfo(primaryMonitor, &primaryMi);

	uint32_t primaryWidth = primaryMi.rcMonitor.right - primaryMi.rcMonitor.left;
	uint32_t primaryHeight = primaryMi.rcMonitor.bottom - primaryMi.rcMonitor.top;

	DEBUG_LOG_FMT("[DxSwapChain] Primary Monitor: {}x{}\n", primaryWidth, primaryHeight);

	auto bestMode = FindBestModeForPrimaryMonitor();
	if (bestMode.width > 0)
	{
		DEBUG_LOG_FMT(
			"[DxSwapChain] Found best mode for primary monitor: {}x{} @ {}Hz\n", bestMode.width, bestMode.height,
			bestMode.refreshRateNumerator / bestMode.refreshRateDenominator
		);
		return bestMode;
	}

	DEVMODE devMode = {};
	devMode.dmSize = sizeof(DEVMODE);
	uint32_t systemRefreshRate = 60;

	if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode))
	{
		systemRefreshRate = devMode.dmDisplayFrequency;
		DEBUG_LOG_FMT("[DxSwapChain] System refresh rate: {}Hz\n", systemRefreshRate);
	}

	DisplayModeInfo fallbackMode{
		.width = primaryWidth,
		.height = primaryHeight,
		.refreshRateNumerator = systemRefreshRate,
		.refreshRateDenominator = 1,
		.format = m_format
	};

	DEBUG_LOG_FMT(
		"[DxSwapChain] Using primary monitor fallback: {}x{} @ {}Hz\n", fallbackMode.width, fallbackMode.height,
		systemRefreshRate
	);

	return fallbackMode;
}

bool DxSwapChain::IsValidResolution(uint32_t width, uint32_t height) const
{
	if (width < 800 || height < 600)
		return false;
	if (width > 7680 || height > 4320)
		return false;

	float aspectRatio = static_cast<float>(width) / height;
	if (aspectRatio < 1.0f || aspectRatio > 3.0f)
		return false;

	std::vector<std::pair<uint32_t, uint32_t>> commonResolutions = {{7680, 4320}, {3840, 2160}, {2560, 1440},
																	{1920, 1080}, {1680, 1050}, {1600, 900},
																	{1366, 768},  {1280, 720}};

	for (const auto& [w, h] : commonResolutions)
	{
		if (width == w && height == h)
			return true;
	}

	for (const auto& [w, h] : commonResolutions)
	{
		for (float scale : {1.25f, 1.5f, 1.75f, 2.0f})
		{
			uint32_t scaledW = static_cast<uint32_t>(w * scale);
			uint32_t scaledH = static_cast<uint32_t>(h * scale);
			if (std::abs(static_cast<int>(width - scaledW)) <= 10 && std::abs(static_cast<int>(height - scaledH)) <= 10)
			{
				return true;
			}
		}
	}

	return false;
}

DxSwapChain::DisplayModeInfo DxSwapChain::FindBestModeForPrimaryMonitor() const
{
	DisplayModeInfo bestMode{};
	uint32_t		maxPixels = 0;
	uint32_t		maxRefreshRate = 0;

	DEBUG_LOG_FMT(
		"[DxSwapChain] Finding best mode for primary monitor from {} available modes\n", m_supportedModes.size()
	);

	for (const auto& mode : m_supportedModes)
	{
		if (!IsValidResolution(mode.width, mode.height))
		{
			continue;
		}

		uint32_t pixels = mode.width * mode.height;
		uint32_t refreshRate = mode.refreshRateNumerator / mode.refreshRateDenominator;

		bool isBetterMode = false;
		if (pixels > maxPixels)
		{
			isBetterMode = true;
			DEBUG_LOG_FMT(
				"[DxSwapChain] Higher resolution found: {}x{} @ {}Hz ({}Mp)\n", mode.width, mode.height, refreshRate,
				pixels / 1'000'000
			);
		}
		else if (pixels == maxPixels && refreshRate > maxRefreshRate)
		{
			isBetterMode = true;
			DEBUG_LOG_FMT(
				"[DxSwapChain] Same resolution, higher refresh rate: {}x{} @ {}Hz\n", mode.width, mode.height,
				refreshRate
			);
		}

		if (isBetterMode)
		{
			bestMode = mode;
			maxPixels = pixels;
			maxRefreshRate = refreshRate;
		}
	}

	if (bestMode.width > 0)
	{
		DEBUG_LOG_FMT(
			"[DxSwapChain] Selected best mode: {}x{} @ {}Hz ({}Mp)\n", bestMode.width, bestMode.height, maxRefreshRate,
			maxPixels / 1'000'000
		);
	}

	return bestMode;
}

void DxSwapChain::StoreWindowedState()
{
	GetWindowRect(m_hwnd, &m_windowedRect);
	m_windowedStyle = GetWindowLongPtr(m_hwnd, GWL_STYLE);
	m_windowedExStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
}

void DxSwapChain::RestoreWindowedState()
{
	SetWindowLongPtr(m_hwnd, GWL_STYLE, m_windowedStyle);
	SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, m_windowedExStyle);

	SetWindowPos(
		m_hwnd, HWND_NOTOPMOST, m_windowedRect.left, m_windowedRect.top, m_windowedRect.right - m_windowedRect.left,
		m_windowedRect.bottom - m_windowedRect.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW
	);

	OnResize(
		m_device, m_windowedRect.right - m_windowedRect.left, m_windowedRect.bottom - m_windowedRect.top,
		m_rtvDescriptorStart, m_rtvDescriptorSize
	);
}

void DxSwapChain::LogMonitorInfo() const
{
	DEBUG_LOG_FMT("[DxSwapChain] === Monitor Information ===\n");

	DEBUG_LOG_FMT("[DxSwapChain] Monitor Count: {}\n", GetSystemMetrics(SM_CMONITORS));
	DEBUG_LOG_FMT(
		"[DxSwapChain] Virtual Screen: {}x{}\n", GetSystemMetrics(SM_CXVIRTUALSCREEN),
		GetSystemMetrics(SM_CYVIRTUALSCREEN)
	);
	DEBUG_LOG_FMT(
		"[DxSwapChain] Primary Screen: {}x{}\n", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)
	);

	HDC hdc = GetDC(nullptr);
	if (hdc)
	{
		int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
		int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
		DEBUG_LOG_FMT("[DxSwapChain] System DPI: {}x{}\n", dpiX, dpiY);
		ReleaseDC(nullptr, hdc);
	}

	DEBUG_LOG_FMT("[DxSwapChain] ===============================\n");
}