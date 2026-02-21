#pragma once
#include "Singleton.h"
#include "DxCommon.h"

class DxSamplerHeapGlobal : public Singleton<DxSamplerHeapGlobal>
{
private:
	friend class Singleton<DxSamplerHeapGlobal>;
	DxSamplerHeapGlobal() = default;
	~DxSamplerHeapGlobal() override = default;

public:
	void Initialize() override;
	void Initialize(ID3D12Device* device, uint32_t maxSamplerCount = 2048);
	void Release() override;

	ID3D12DescriptorHeap* GetHeap() const { return m_heap.Get(); }

private:
	ComPtr<ID3D12DescriptorHeap> m_heap;
	uint32_t                     m_descriptorIncrementSize = 0;
};
