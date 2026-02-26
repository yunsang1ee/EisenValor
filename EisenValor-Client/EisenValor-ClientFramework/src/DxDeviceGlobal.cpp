#include "stdafxClientFramework.h"
#include "DxDeviceGlobal.h"
#include "DxCommon.h"

void DxDeviceGlobal::Initialize()
{
	uint32_t dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

	ComPtr<IDXGIAdapter1> tempAdapter;
	SIZE_T				  maxDedicatedVideoMemory = 0;

	for (uint32_t i = 0; m_factory->EnumAdapters1(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc;
		tempAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)) &&
			desc.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
			tempAdapter.As(&m_adapter);

			DEBUG_LOG_FMT(
				"[DxDevice] Adapter candidate: {} ({} MB VRAM)\n",
				std::string(std::begin(desc.Description), std::end(desc.Description)),
				desc.DedicatedVideoMemory / (1024 * 1024)
			);
		}
	}

	assert(m_adapter != nullptr && "[DxDevice] Failed to find a suitable DXGI Adapter");

	{
		DXGI_ADAPTER_DESC1 finalDesc;
		m_adapter->GetDesc1(&finalDesc);
		DEBUG_LOG_FMT(
			"[DxDevice] Selected adapter: {} ({} MB VRAM)\n",
			std::string(std::begin(finalDesc.Description), std::end(finalDesc.Description)),
			finalDesc.DedicatedVideoMemory / (1024 * 1024)
		);
	}

	ThrowIfFailed(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));

	DEBUG_LOG_FMT("[DxDevice] Device created successfully.\n");
}

void DxDeviceGlobal::Release()
{
#ifdef _DEBUG
	if (m_device)
	{
		ULONG refCount = m_device->AddRef() - 1;
		m_device->Release();
		if (refCount > 1)
		{
			DEBUG_LOG_FMT("[DxDevice] Warning: Device still has {} references!\n", refCount - 1);
		}
	}
#endif

	m_device.Reset();
	m_adapter.Reset();
	m_factory.Reset();

	DEBUG_LOG_FMT("[DxDevice] Released DxDeviceGlobal.\n");
}
