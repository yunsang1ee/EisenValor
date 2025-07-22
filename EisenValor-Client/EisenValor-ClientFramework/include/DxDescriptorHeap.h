#pragma once

struct DescriptorHandles
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
};

class DxDescriptorHeap
{
public:
	DxDescriptorHeap(ID3D12Device* device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32_t descriptorCount,
		bool shaderVisible = false);

	DxDescriptorHeap(const DxDescriptorHeap&) = delete;
	DxDescriptorHeap& operator=(const DxDescriptorHeap&) = delete;

	DescriptorHandles Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const;

	ID3D12DescriptorHeap* GetHeap() const { return m_heap.Get(); }
	uint32_t GetDescriptorSize() const { return m_descriptorSize; }
	uint32_t GetCapacity() const { return m_capacity; }
	uint32_t GetAllocatedCount() const { return m_allocIndex; }

	void Reset();

private:
	ComPtr<ID3D12DescriptorHeap> m_heap;

	uint32_t m_descriptorSize = 0;
	uint32_t m_capacity = 0;
	uint32_t m_allocIndex = 0;

	D3D12_DESCRIPTOR_HEAP_TYPE m_type;
	bool m_shaderVisible = false;

	D3D12_CPU_DESCRIPTOR_HANDLE m_baseCPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_baseGPU = {};
};

