#include "stdafxClientFramework.h"
#include "DxDeviceGlobal.h"
#include "DxDebugGlobal.h"
#include "DxCommon.h"

void DxDeviceGlobal::Initialize()
{
	uint32_t dxgiFactoryFlags = 0;

#ifdef _DEBUG
	GlobalRegistry::Get<IDxDebugGlobal>().EnableDebug();
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

	ComPtr<IDXGIAdapter1> tempAdapter;
	SIZE_T maxDedicatedVideoMemory = 0;

	for (uint32_t i = 0; m_factory->EnumAdapters1(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc;
		tempAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr))
			&& desc.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
			tempAdapter.As(&m_adapter);
		}
	}

	assert(m_adapter != nullptr && "[DxDevice] Failed to find a suitable DXGI Adapter");

	ThrowIfFailed(D3D12CreateDevice(
		m_adapter.Get(),
		D3D_FEATURE_LEVEL_12_1,
		IID_PPV_ARGS(&m_device)
	));

	DEBUG_LOG_FMT("[DxDevice] Device created successfully.\n");
}
