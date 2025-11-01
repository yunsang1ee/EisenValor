#pragma once
#include "Singleton.h"
#include "DxCommon.h"

class DxDeviceGlobal : public Singleton<DxDeviceGlobal>
{
private:
	friend class Singleton<DxDeviceGlobal>;

	DxDeviceGlobal() = default;
	~DxDeviceGlobal() override = default;

public:
	void Initialize() override;
	void Release() override;

	ID3D12Device*  GetDevice() const { return m_device.Get(); }
	IDXGIFactory6* GetFactory() const { return m_factory.Get(); }
	IDXGIAdapter4* GetAdapter() const { return m_adapter.Get(); }

private:
	ComPtr<ID3D12Device>  m_device;
	ComPtr<IDXGIFactory6> m_factory;
	ComPtr<IDXGIAdapter4> m_adapter;
};