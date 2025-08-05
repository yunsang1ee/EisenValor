#pragma once

class IDxDeviceGlobal : public IGlobal
{
public:
	virtual void		   Initialize() = 0;
	virtual ID3D12Device*  GetDevice() const = 0;
	virtual IDXGIFactory6* GetFactory() const = 0;
	virtual IDXGIAdapter4* GetAdapter() const = 0;
};

class DxDeviceGlobal : public GlobalMakerBase<DxDeviceGlobal, IDxDeviceGlobal>
{
public:
	void Initialize() override;

	ID3D12Device*  GetDevice() const override { return m_device.Get(); }
	IDXGIFactory6* GetFactory() const override { return m_factory.Get(); }
	IDXGIAdapter4* GetAdapter() const override { return m_adapter.Get(); }

private:
	ComPtr<ID3D12Device>  m_device;
	ComPtr<IDXGIFactory6> m_factory;
	ComPtr<IDXGIAdapter4> m_adapter;
};