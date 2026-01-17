#pragma once

class DxDescriptorHeapGlobal;

class DxDescriptorHandles
{
public:
	static constexpr uint32_t kInvalidIndex = ~0u;

	DxDescriptorHandles() = default;
	DxDescriptorHandles(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu, uint32_t index)
		: m_cpuHandle(cpu), m_gpuHandle(gpu), m_index(index)
	{
	}

	DxDescriptorHandles(const DxDescriptorHandles&) = delete;
	DxDescriptorHandles& operator=(const DxDescriptorHandles&) = delete;

	DxDescriptorHandles(DxDescriptorHandles&& other) noexcept
		: m_cpuHandle(other.m_cpuHandle), m_gpuHandle(other.m_gpuHandle), m_index(other.m_index)
	{
		other.Invalidate();
	}
	DxDescriptorHandles& operator=(DxDescriptorHandles&& other) noexcept
	{
		if (this != &other)
		{
			assert(m_index == kInvalidIndex && "[DescriptorHandle] Overwriting valid handle without releasing!");
			m_cpuHandle = other.m_cpuHandle;
			m_gpuHandle = other.m_gpuHandle;
			m_index = other.m_index;
			other.Invalidate();
		}
		return *this;
	}

public:
	void Free(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle, std::string_view debugName);
	void FreeImmediate(DxDescriptorHeapGlobal& heap);

	bool						IsValid() const { return m_index != kInvalidIndex; }
	uint32_t					GetIndex() const { return m_index; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_cpuHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return m_gpuHandle; }

	operator uint32_t() const { return m_index; }

	void Invalidate()
	{
		m_index = kInvalidIndex;
		m_cpuHandle = {};
		m_gpuHandle = {};
	}

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle{};
	uint32_t					m_index = kInvalidIndex;
};

struct DxDescriptorRange
{
	uint32_t startIndex;
	uint32_t count;
};

class DxDescriptorTableBuilder
{
public:
	using DescriptorCreator = std::function<void(ID3D12Device*, DxDescriptorHeapGlobal&, uint32_t)>;

	void AddSRV(
		ID3D12Resource*						   resource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr,
		DxDescriptorHandles*				   outputHandle = nullptr
	);

	void AddUAV(
		ID3D12Resource*							resource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr,
		ID3D12Resource*							counterRes = nullptr,
		DxDescriptorHandles*					outputHandle = nullptr
	);

	void AddCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, DxDescriptorHandles* outputHandle = nullptr);

	DxDescriptorRange Commit(ID3D12Device* device, DxDescriptorHeapGlobal& heap);

private:
	std::vector<DescriptorCreator> m_creators;
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


	DxDescriptorHandles Allocate();
	void				FreeImmediate(uint32_t index);
	void				Free(uint32_t index, const FenceHandle& fenceHandle, std::string_view debugName);

	DxDescriptorRange ReserveRange(uint32_t count);
	void			  FreeRangeImmediate(uint32_t startIndex, uint32_t count);
	void FreeRange(uint32_t startIndex, uint32_t count, const FenceHandle& fenceHandle, std::string_view debugName);


	DxDescriptorHandles CreateSRV(
		ID3D12Device* device, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
	);
	DxDescriptorHandles CreateUAV(
		ID3D12Device* device, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc
	);
	DxDescriptorHandles CreateCBV(ID3D12Device* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc);

	void CreateSRVAt(
		ID3D12Device* device, uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
	);

	DxDescriptorRange CreateSRVRange(
		ID3D12Device* device, const std::vector<ID3D12Resource*>& resources, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
	);

	DxDescriptorRange CreateSRVRange(
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
	uint32_t		  AllocateOneFromFreeRanges();
	DxDescriptorRange ReserveRangeFromFreeRanges(uint32_t);
	void			  InsertFreeRangeAndMerge(uint32_t begin, uint32_t end);
	void			  SweepFreeSlotsIntoRanges();

private:
	static constexpr uint32_t kInvalidIndex = DxDescriptorHandles::kInvalidIndex;

	ComPtr<ID3D12DescriptorHeap> m_heap;

	uint32_t m_descriptorIncrementSize = 0;
	uint32_t m_capacity = 0;
	uint32_t m_allocIndex = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE m_baseCPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_baseGPU{};

	std::vector<uint32_t> m_freeSlots;
	struct FreeRangeEntry
	{
		uint32_t begin, end;
	};
	std::vector<FreeRangeEntry> m_freeRanges;

#ifdef _DEBUG
	std::vector<uint8_t> m_liveMarkers; // 0=free, 1=allocated
#endif
};
