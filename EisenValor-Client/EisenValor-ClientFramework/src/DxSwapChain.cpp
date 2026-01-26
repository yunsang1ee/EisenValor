#include "stdafxClientFramework.h"
#include "DxSwapChain.h"
#include "DxUtils.h"
#include <DxCommandQueueGlobal.h>

DxSwapChain::DxSwapChain(
	ID3D12Device*				device,
	IDXGIFactory6*				factory,
	DxGfxCommandQueueGlobal&	commandQueue,
	HWND						hwnd,
	uint32_t					width,
	uint32_t					height,
	uint32_t					backBufferCount,
	DXGI_FORMAT					format,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart,
	uint32_t					rtvDescriptorSize
)
	: m_device(device), m_factory(factory), m_gfxCommandQueueGlobal(commandQueue), m_hwnd(hwnd), m_width(width),
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
		m_gfxCommandQueueGlobal.GetQueue(), m_hwnd, &swapChainDesc,
		nullptr, // Fullscreen Optimizations
		nullptr, &swapChain1
	));

	ThrowIfFailed(swapChain1.As(&m_swapChain));
	ThrowIfFailed(m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

	CreateResources(device, m_gfxCommandQueueGlobal.GetQueue(), m_rtvDescriptorStart, m_rtvDescriptorSize);

	m_nativeDisplayMode = GetNativeDisplayMode();

	EnumerateDisplayModes();
	m_currentDisplayMode = FindBestModeForCurrentMonitor();

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
	m_gfxCommandQueueGlobal.WaitForIdle(); // Ensure all GPU operations are complete before releasing resources
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

	CreateResources(device, m_gfxCommandQueueGlobal.GetQueue(), m_rtvDescriptorStart, m_rtvDescriptorSize);

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
		.refreshRate = m_currentDisplayMode.refreshRateDenominator != 0
						   ? (m_currentDisplayMode.refreshRateNumerator / m_currentDisplayMode.refreshRateDenominator)
						   : 60,
	};
}

void DxSwapChain::SetFullscreen(bool enable)
{
	if (m_isBorderlessFullscreen || m_isFullscreen == enable)
		return;

	if (!m_canExclusiveFullscreen || !m_outputFoundLastEnum)
	{
		DEBUG_LOG_FMT("[DxSwapChain] Exclusive fullscreen unavailable. Fallback to borderless.\n");
		SetBorderlessFullscreen(enable);
		return;
	}

	HRESULT hr = m_swapChain->SetFullscreenState(enable ? TRUE : FALSE, nullptr);
	if (FAILED(hr))
	{
		DEBUG_LOG_FMT(
			"[DxSwapChain] SetFullscreenState({}) failed: 0x{:08X}\n", enable ? "TRUE" : "FALSE", (uint32_t)hr
		);

		BOOL isFs = FALSE;
		if (SUCCEEDED(m_swapChain->GetFullscreenState(&isFs, nullptr)))
		{
			m_isFullscreen = (isFs == TRUE);
			DEBUG_LOG_FMT(
				"[DxSwapChain] Actual fullscreen state after failure: {}\n", m_isFullscreen ? "TRUE" : "FALSE"
			);
		}

		if (enable == FALSE)
		{
			DEBUG_LOG_FMT("[DxSwapChain] Exit exclusive failed. Please try again.\n");
			return;
		}

		m_canExclusiveFullscreen = false;
		SetBorderlessFullscreen(enable);
		return;
	}

	m_isFullscreen = enable;
	DEBUG_LOG_FMT("[DxSwapChain] Exclusive Fullscreen: {}\n", m_isFullscreen ? "TRUE" : "FALSE");
	return;
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

bool DxSwapChain::TryFindOutputFromDeviceAdapter(ComPtr<IDXGIOutput>& outOutput) const
{
	outOutput.Reset();

	const HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
	if (!monitor)
	{
		return false;
	}

	const LUID			  luid = m_device->GetAdapterLuid();
	ComPtr<IDXGIFactory4> factory4;
	if (FAILED(m_factory->QueryInterface(IID_PPV_ARGS(&factory4))) || !factory4)
	{
		return false;
	}

	ComPtr<IDXGIAdapter> adapter;
	if (FAILED(factory4->EnumAdapterByLuid(luid, IID_PPV_ARGS(&adapter))))
	{
		return false;
	}


	for (UINT i = 0;; ++i)
	{
		ComPtr<IDXGIOutput> output;
		if (adapter->EnumOutputs(i, &output) == DXGI_ERROR_NOT_FOUND)
		{
			break;
		}
		if (!output)
		{
			continue;
		}

		DXGI_OUTPUT_DESC desc{};
		if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == monitor)
		{
			outOutput = output;
			return true;
		}
	}
	return false;
}

bool DxSwapChain::TryFindOutputFromFactory(ComPtr<IDXGIOutput>& outOutput) const
{
	outOutput.Reset();
	const HMONITOR target = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

	for (UINT adaptorIdx = 0;; ++adaptorIdx)
	{
		ComPtr<IDXGIAdapter1> adaptor;
		if (m_factory->EnumAdapters1(adaptorIdx, &adaptor) == DXGI_ERROR_NOT_FOUND)
		{
			break;
		}
		if (!adaptor)
		{
			continue;
		}

		for (UINT outputIdx = 0;; ++outputIdx)
		{
			ComPtr<IDXGIOutput> output;
			if (adaptor->EnumOutputs(outputIdx, &output) == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}
			if (!output)
			{
				continue;
			}

			DXGI_OUTPUT_DESC desc{};
			if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == target)
			{
				outOutput = output;
				return true;
			}
		}
	}
	return false;
}

bool DxSwapChain::TryGetOutput(ComPtr<IDXGIOutput>& outOutput) const
{
	outOutput.Reset();

	if (SUCCEEDED(m_swapChain->GetContainingOutput(&outOutput)) && outOutput)
	{
		return true;
	}

	if (TryFindOutputFromDeviceAdapter(outOutput))
	{
		return true;
	}

	if (TryFindOutputFromFactory(outOutput))
	{
		return true;
	}

	return false;
}

void DxSwapChain::EnumerateDisplayModes()
{
	m_supportedModes.clear();

	ComPtr<IDXGIOutput> output;
	const bool			outputFound = TryGetOutput(output);

	m_outputFoundLastEnum = outputFound;
	if (outputFound)
	{
		UINT	numModes = 0;
		HRESULT hr = output->GetDisplayModeList(m_format, 0, &numModes, nullptr);

		DEBUG_LOG_FMT("[DxSwapChain] Found {} display modes for format {}\n", numModes, (int)m_format);

		if (SUCCEEDED(hr) && numModes > 0)
		{
			std::vector<DXGI_MODE_DESC> modes(numModes);
			if (SUCCEEDED(output->GetDisplayModeList(m_format, 0, &numModes, modes.data())))
			{
				for (const auto& mode : modes)
				{
					if (IsValidResolution(mode.Width, mode.Height))
					{
						m_supportedModes.emplace_back(
							mode.Width, mode.Height, mode.RefreshRate.Numerator, mode.RefreshRate.Denominator,
							mode.Format, mode.Scaling
						);
					}
				}
			}
		}
		DEBUG_LOG_FMT("[DxSwapChain] Enumerated {} display modes (DXGI output)\n", m_supportedModes.size());
		return;
	}
	else
	{
		DEBUG_LOG_FMT("[DxSwapChain] No DXGI output found. Falling back to Win32 API.\n");
		EnumerateDisplayModesWin32();
	}

	if (m_supportedModes.empty())
	{
		DEBUG_LOG_FMT("[DxSwapChain] No valid modes found. Using native display mode as fallback.\n");
		m_supportedModes.push_back(m_nativeDisplayMode);
	}
}

void DxSwapChain::EnumerateDisplayModesWin32()
{
	const HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

	MONITORINFOEXW info{};
	info.cbSize = sizeof(info);
	const bool gotMonitorInfo = (GetMonitorInfoW(monitor, &info) != FALSE);

	if (!gotMonitorInfo)
	{
		DEBUG_LOG_FMT("[DxSwapChain] GetMonitorInfoW failed. Cannot enumerate display modes.\n");
		return;
	}

	std::set<std::tuple<uint32_t, uint32_t, UINT>> uniqueModes;

	for (DWORD i = 0;; ++i)
	{
		DEVMODEW dm{};
		dm.dmSize = sizeof(dm);

		if (!EnumDisplaySettingsExW(info.szDevice, i, &dm, 0))
		{
			break;
		}

		if (dm.dmPelsWidth < 800 || dm.dmPelsHeight < 600)
		{
			continue;
		}

		if (dm.dmBitsPerPel < 24)
		{
			continue;
		}

		UINT freq = 60;
		if (dm.dmDisplayFrequency >= 24 && dm.dmDisplayFrequency <= 500)
		{
			freq = dm.dmDisplayFrequency;
		}

		auto modeKey = std::make_tuple((uint32_t)dm.dmPelsWidth, (uint32_t)dm.dmPelsHeight, freq);

		if (uniqueModes.insert(modeKey).second)
		{
			if (IsValidResolution((uint32_t)dm.dmPelsWidth, (uint32_t)dm.dmPelsHeight))
			{
				m_supportedModes.emplace_back(
					(uint32_t)dm.dmPelsWidth, (uint32_t)dm.dmPelsHeight, freq, 1, m_format,
					DXGI_MODE_SCALING_UNSPECIFIED
				);
			}
		}
	}
	DEBUG_LOG_FMT("[DxSwapChain] Enumerated {} valid display modes (Win32 fallback)\n", m_supportedModes.size());
}

DxSwapChain::DisplayModeInfo DxSwapChain::GetNativeDisplayMode() const
{
	const HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

	MONITORINFOEX info{};
	info.cbSize = sizeof(info);
	if (!GetMonitorInfo(monitor, &info))
	{
		DEBUG_LOG_FMT("[DxSwapChain] GetMonitorInfo failed. Using fallback 1920x1080@60Hz.\n");
		return {1920, 1080, 60, 1, m_format, DXGI_MODE_SCALING_UNSPECIFIED};
	}

	const uint32_t width = info.rcMonitor.right - info.rcMonitor.left;
	const uint32_t height = info.rcMonitor.bottom - info.rcMonitor.top;

	DEVMODE dm{};
	dm.dmSize = sizeof(dm);
	uint32_t refreshRate = 60;
	if (EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &dm))
	{
		refreshRate = dm.dmDisplayFrequency;
	}

	DEBUG_LOG_FMT("[DxSwapChain] Native mode: {}x{} @{}Hz\n", width, height, refreshRate);

	return {width, height, refreshRate, 1, m_format, DXGI_MODE_SCALING_UNSPECIFIED};
}

bool DxSwapChain::IsValidResolution(uint32_t width, uint32_t height) const
{
	if (width == m_nativeDisplayMode.width && height == m_nativeDisplayMode.height)
	{
		return true;
	}

	if (width == 0 || height == 0 || width < 800 || height < 600 || width > 7680 || height > 4320)
	{
		return false;
	}

	const float aspectRatio = static_cast<float>(width) / height;
	if (aspectRatio < 1.0f || aspectRatio > 3.6f)
	{
		return false;
	}


	return true;
}

DxSwapChain::DisplayModeInfo DxSwapChain::FindBestModeForCurrentMonitor() const
{
	DisplayModeInfo bestMode = m_nativeDisplayMode;
	uint32_t		maxRefreshRate = 0;
	uint32_t		maxPixels = m_nativeDisplayMode.width * m_nativeDisplayMode.height;

	
    for (const auto& mode : m_supportedModes)
	{
		const uint32_t pixels = mode.width * mode.height;
		const uint32_t refreshRate =
			(mode.refreshRateDenominator != 0) ? (mode.refreshRateNumerator / mode.refreshRateDenominator) : 0;

		const bool isBetter = (pixels > maxPixels) || (pixels == maxPixels && refreshRate > maxRefreshRate);

		if (isBetter)
		{
			bestMode = mode;
			maxPixels = pixels;
			maxRefreshRate = refreshRate;
		}
	}

	DEBUG_LOG_FMT("[DxSwapChain] Selected best mode: {}x{} @{}Hz\n", bestMode.width, bestMode.height, maxRefreshRate);
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