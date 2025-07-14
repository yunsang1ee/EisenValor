#include "stdafxClientFramework.h"
#include "DxSwapChain.h"

constexpr UINT SWAP_CHAIN_FLAGS = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

DxSwapChain::DxSwapChain(ID3D12Device* device, IDXGIFactory6* factory, ID3D12CommandQueue* commandQueue,
    HWND hwnd, uint32_t width, uint32_t height, uint32_t backBufferCount, DXGI_FORMAT format,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart, uint32_t rtvDescriptorSize)
    : m_device(device), m_factory(factory), m_commandQueue(commandQueue),
    m_hwnd(hwnd), m_width(width), m_height(height), m_backBufferCount(backBufferCount), m_format(format),
    m_rtvDescriptorStart(rtvDescriptorStart), m_rtvDescriptorSize(rtvDescriptorSize)
{
    assert(device && "[DxSwapChain] ID3D12Device is null.");
    assert(factory && "[DxSwapChain] IDXGIFactory6 is null.");
    assert(commandQueue && "[DxSwapChain] ID3D12CommandQueue is null.");
    assert(hwnd && "[DxSwapChain] HWND is null.");
    assert(width > 0 && "[DxSwapChain] Width > 0.");
    assert(height > 0 && "[DxSwapChain] Height > 0.");
    assert(backBufferCount > 0 && "[DxSwapChain] BackBufferCount > 0.");
    assert(rtvDescriptorStart.ptr && "[DxSwapChain] RTV Descriptor Start handle is null.");
    assert(rtvDescriptorSize > 0 && "[DxSwapChain] RTV Descriptor Size > 0.");


    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width = m_width,
        .Height = m_height,
        .Format = m_format,
        .Stereo = FALSE,
        .SampleDesc {
            .Count = 1,
            .Quality = 0
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = m_backBufferCount,
        .Scaling = DXGI_SCALING_NONE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = SWAP_CHAIN_FLAGS
    };

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(m_factory->CreateSwapChainForHwnd(
        m_commandQueue,
        m_hwnd,
        &swapChainDesc,
        nullptr, // Fullscreen Optimizations
        nullptr,
        &swapChain1
    ));

    ThrowIfFailed(swapChain1.As(&m_swapChain));
	ThrowIfFailed(m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER)); //TODO: Alt+Enter Input Processing in InputGlobal

    CreateResources(device, m_commandQueue, m_rtvDescriptorStart, m_rtvDescriptorSize);

    DEBUG_LOG_FMT("[DxSwapChain] SwapChain created: %dx%d, BackBuffers: %d, Format: %d\n",
        m_width, m_height, m_backBufferCount, (int)m_format);
}

DxSwapChain::~DxSwapChain()
{
    ReleaseBackBuffers();
    DEBUG_LOG_FMT("[DxSwapChain] DxSwapChain destroyed.\n");
}

void DxSwapChain::ReleaseBackBuffers()
{
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
    ThrowIfFailed(m_swapChain->Present(syncInterval, flags));
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}
void DxSwapChain::OnResize(ID3D12Device* device, uint32_t newWidth, uint32_t newHeight,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart, uint32_t rtvDescriptorSize)
{
    if (newWidth == 0 || newHeight == 0)
    {
        DEBUG_LOG_FMT("[DxSwapChain] OnResize called with 0 width/height. Skipping resize.\n");
        return;
    }

    ReleaseBackBuffers();
    ThrowIfFailed(m_swapChain->ResizeBuffers(
        m_backBufferCount,
        newWidth,
        newHeight,
        m_format,
        SWAP_CHAIN_FLAGS
    ));

    m_rtvDescriptorStart = rtvDescriptorStart;
    m_rtvDescriptorSize = rtvDescriptorSize;
    m_width = newWidth;
    m_height = newHeight;

    CreateResources(device, m_commandQueue, m_rtvDescriptorStart, m_rtvDescriptorSize);

    DEBUG_LOG_FMT("[DxSwapChain] SwapChain resized to %dx%d. BackBuffers recreated.\n", m_width, m_height);
}

void DxSwapChain::CreateResources(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorStart, uint32_t rtvDescriptorSize)
{
    m_backBuffers.resize(m_backBufferCount);

    for (uint32_t i = 0; i < m_backBufferCount; ++i)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorStart;
        rtvHandle.ptr += static_cast<size_t>(i) * rtvDescriptorSize;

        device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
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
        DEBUG_LOG_FMT("[DxSwapChain] Warning: Invalid back buffer index %d. Max is %d.\n", index, m_backBufferCount - 1);
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
        DEBUG_LOG_FMT("[DxSwapChain] Warning: Invalid RTV index %d. Max is %d.\n", index, m_backBufferCount - 1);
        return {};
    }
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorStart;
    rtvHandle.ptr += static_cast<size_t>(index) * m_rtvDescriptorSize;
    return rtvHandle;
}