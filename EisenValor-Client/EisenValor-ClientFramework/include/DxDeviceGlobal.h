#pragma once
#include "Singleton.h"
#include "DxCommon.h"

#include <thread>
#include <atomic>

class DeviceRemovedMonitor
{
public:
	DeviceRemovedMonitor() = default;
	~DeviceRemovedMonitor() { Stop(); }

	void Start(ID3D12Device* device, ID3D12Fence* fence);
	void Stop();

	void DumpDred(ID3D12Device* device);

private:
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Fence>	 m_fence;
	HANDLE				 m_event = nullptr;
	std::thread			 m_thread;
	std::atomic<bool>	 m_running{false};
};

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

	DeviceRemovedMonitor& GetMonitor() { return m_monitor; }

private:
	ComPtr<ID3D12Device>  m_device;
	ComPtr<IDXGIFactory6> m_factory;
	ComPtr<IDXGIAdapter4> m_adapter;

	DeviceRemovedMonitor m_monitor;
};