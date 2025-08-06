#pragma once

class IDxGraphicsCommandQueueGlobal;

class DxSwapChain
{
public:
	struct SwapChainInfo
	{
		uint32_t	width;
		uint32_t	height;
		uint32_t	backBufferCount;
		DXGI_FORMAT format;
		uint32_t	currentBackBufferIndex;
		bool		isFullscreen;
		bool		isBorderlessFullscreen;
		bool		supportsTearing;
		uint64_t	frameCount;
		uint32_t	refreshRate;
	};

	struct DisplayModeInfo
	{
		uint32_t		  width;
		uint32_t		  height;
		uint32_t		  refreshRateNumerator;
		uint32_t		  refreshRateDenominator;
		DXGI_FORMAT		  format;
		DXGI_MODE_SCALING scaling;
	};

	DxSwapChain(
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
	);
	~DxSwapChain();

	DxSwapChain(const DxSwapChain&) = delete;
	DxSwapChain& operator=(const DxSwapChain&) = delete;

	void Present(UINT syncInterval, UINT flags);
	void PresentMaxPerformance();

	void OnResize(
		ID3D12Device*				device,
		uint32_t					newWidth,
		uint32_t					newHeight,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart,
		uint32_t					rtvDescriptorSize
	);

	ID3D12Resource* GetCurrentBackBuffer() const;
	ID3D12Resource* GetBackBuffer(uint32_t index) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(uint32_t index) const;
	uint32_t					GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	uint32_t GetBackBufferCount() const { return m_backBufferCount; }

	SwapChainInfo GetInfo() const;
	uint64_t	  GetFrameCount() const { return m_frameCount; }


	void SetBorderlessFullscreen(bool enable);
	void ToggleBorderlessFullscreen() { SetBorderlessFullscreen(!m_isBorderlessFullscreen); }
	bool IsBorderlessFullscreen() const { return m_isBorderlessFullscreen; }

	void SetFullscreen(bool enable);
	void ToggleFullscreen() { SetFullscreen(!m_isFullscreen); }
	bool IsFullscreen() const { return m_isFullscreen; }

	std::vector<DisplayModeInfo> GetSupportedDisplayModes() const { return m_supportedModes; }
	DisplayModeInfo				 GetCurrentDisplayMode() const { return m_currentDisplayMode; }

private:
	void CreateResources(
		ID3D12Device*				device,
		ID3D12CommandQueue*			commandQueue,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart,
		uint32_t					rtvDescriptorSize
	);
	void ReleaseBackBuffers();

	void			EnumerateDisplayModes();
	DisplayModeInfo GetNativeDisplayMode() const;
	DisplayModeInfo FindBestModeForPrimaryMonitor() const;
	bool			IsValidResolution(uint32_t width, uint32_t height) const;
	void			StoreWindowedState();
	void			RestoreWindowedState();

	void LogMonitorInfo() const;

private:
	ID3D12Device*				   m_device = nullptr;
	IDXGIFactory6*				   m_factory = nullptr;
	HWND						   m_hwnd = nullptr;
	IDxGraphicsCommandQueueGlobal& m_graphicsCommandQueueGlobal;

	ComPtr<IDXGISwapChain4> m_swapChain;

	std::vector<ComPtr<ID3D12Resource>> m_backBuffers;
	uint32_t							m_currentBackBufferIndex = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvDescriptorStart{};
	uint32_t					m_rtvDescriptorSize = 0;

	uint32_t	m_width{};
	uint32_t	m_height{};
	uint32_t	m_backBufferCount{};
	DXGI_FORMAT m_format{};
	UINT		m_swapChainFlags = 0;

	uint64_t m_frameCount = 0;
	bool	 m_supportsTearing = false;

	bool m_isFullscreen = false;
	bool m_isBorderlessFullscreen = false;

	RECT	 m_windowedRect{};
	LONG_PTR m_windowedStyle = 0;
	LONG_PTR m_windowedExStyle = 0;

	std::vector<DisplayModeInfo> m_supportedModes;
	DisplayModeInfo				 m_currentDisplayMode{};
};