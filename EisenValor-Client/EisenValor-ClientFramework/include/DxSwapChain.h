#pragma once

class IDxGraphicsCommandQueueGlobal;

class DxSwapChain
{
public:
	DxSwapChain(ID3D12Device* device, IDXGIFactory6* factory, IDxGraphicsCommandQueueGlobal& commandQueue,
		HWND hwnd, uint32_t width, uint32_t height, uint32_t backBufferCount, DXGI_FORMAT format,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart, uint32_t rtvDescriptorSize);
	~DxSwapChain();

	DxSwapChain(const DxSwapChain&) = delete;
	DxSwapChain& operator=(const DxSwapChain&) = delete;

	void Present(UINT syncInterval, UINT flags);

	// All GPU operations must complete before resizing. recommended to call DxCommandContextPool::WaitForGPU().
	void OnResize(ID3D12Device* device, uint32_t newWidth, uint32_t newHeight,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart, uint32_t rtvDescriptorSize);

	ID3D12Resource* GetCurrentBackBuffer() const;
	ID3D12Resource* GetBackBuffer(uint32_t index) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(uint32_t index) const;
	uint32_t GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	uint32_t GetBackBufferCount() const { return m_backBufferCount; }

private:
	void CreateResources(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart, uint32_t rtvDescriptorSize);

	void ReleaseBackBuffers();

private:
	ID3D12Device*       m_device = nullptr;
	IDXGIFactory6*      m_factory = nullptr;
	IDxGraphicsCommandQueueGlobal& m_graphicsCommandQueueGlobal;
	HWND                m_hwnd = nullptr;

	ComPtr<IDXGISwapChain4> m_swapChain;

	std::vector<ComPtr<ID3D12Resource>> m_backBuffers;
	uint32_t m_currentBackBufferIndex = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvDescriptorStart = {};
	uint32_t                    m_rtvDescriptorSize = 0;

	uint32_t    m_width;
	uint32_t    m_height;
	uint32_t    m_backBufferCount;
	DXGI_FORMAT m_format;
};