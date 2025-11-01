#pragma once

struct DescriptorHandles
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	uint32_t					index{};
};

struct BatchAllocation
{
	uint32_t startIndex;
	uint32_t count;
};

class DxDescriptorHeapGlobal : public Singleton<DxDescriptorHeapGlobal>
{
private:
	friend class Singleton<DxDescriptorHeapGlobal>;
	DxDescriptorHeapGlobal() = default;
	~DxDescriptorHeapGlobal() override = default;

public:
	void Initialize() override;
	void Initialize(ID3D12Device* device, uint32_t maxDescriptorCount = 1'000'000);
	void Release() override;

	DxDescriptorHeapGlobal(const DxDescriptorHeapGlobal&) = delete;
	DxDescriptorHeapGlobal& operator=(const DxDescriptorHeapGlobal&) = delete;


	DescriptorHandles Allocate();
	void			  FreeImmediate(uint32_t globalIndex);
	void			  Free(uint32_t index, const FenceHandle& fenceHandle, std::string_view debugName);

	BatchAllocation ReserveBatch(uint32_t count);
	void			FreeBatchImmediate(uint32_t startIndex, uint32_t count);
	void FreeBatch(uint32_t startIndex, uint32_t count, const FenceHandle& fenceHandle, std::string_view debugName);


	uint32_t CreateSRV(ID3D12Device* device, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc);
	uint32_t CreateUAV(ID3D12Device* device, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc);
	uint32_t CreateCBV(ID3D12Device* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc);

	void CreateSRVAt(
		ID3D12Device*						   device,
		uint32_t							   globalIndex,
		ID3D12Resource*						   resource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
	);

	BatchAllocation CreateSRVBatch(
		ID3D12Device* device, const std::vector<ID3D12Resource*>& resources, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
	);

	BatchAllocation CreateSRVBatch(
		ID3D12Device*										device,
		const std::vector<ID3D12Resource*>&					resources,
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>& descs
	);


	ID3D12DescriptorHeap*		GetHeap() const { return m_heap.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleStart() const { return m_baseGPU; }
	uint32_t					GetDescriptorSize() const { return m_descriptorIncrementSize; }
	uint32_t					GetCapacity() const { return m_capacity; }
	uint32_t					GetAllocatedCount() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const;


	void Reset();
	void LogStats() const;

private:
	uint32_t		AllocateOneFromFreeRanges();
	BatchAllocation ReserveBatchFromFreeRanges(uint32_t);
	void			InsertFreeRangeAndMerge(uint32_t begin, uint32_t end);
	void			SweepFreeSlotsIntoRanges();

private:
	ComPtr<ID3D12DescriptorHeap> m_heap;

	uint32_t m_descriptorIncrementSize = 0;
	uint32_t m_capacity = 0;
	uint32_t m_allocIndex = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE m_baseCPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_baseGPU{};

	std::vector<uint32_t> m_freeSlots;
	struct FreeRange
	{
		uint32_t begin, end;
	};
	std::vector<FreeRange> m_freeRanges;

#ifdef _DEBUG
	std::vector<uint8_t> m_liveMarkers; // 0=free, 1=allocated
#endif
};
