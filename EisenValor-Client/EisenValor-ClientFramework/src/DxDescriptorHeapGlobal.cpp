#include "stdafxClientFramework.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxUtils.h"
#include <string>

void DxDescriptorHandles::Free(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle, std::string_view debugName)
{
	if (IsValid())
	{
		heap.Free(m_index, fenceHandle, debugName);
		Invalidate();
	}
}

void DxDescriptorHandles::FreeImmediate(DxDescriptorHeapGlobal& heap)
{
	if (IsValid())
	{
		heap.FreeImmediate(m_index);
		Invalidate();
	}
}

void DxDescriptorTableBuilder::AddSRV(
	ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc, DxDescriptorHandles* outputHandle
)
{
	bool							hasDesc = (desc != nullptr);
	D3D12_SHADER_RESOURCE_VIEW_DESC descCopy = hasDesc ? *desc : D3D12_SHADER_RESOURCE_VIEW_DESC{};

	m_creators.emplace_back(
		[=](ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t index)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap.GetCPUHandle(index);

			device->CreateShaderResourceView(resource, hasDesc ? &descCopy : nullptr, cpuHandle);

			if (outputHandle)
			{
				*outputHandle = DxDescriptorHandles(cpuHandle, heap.GetGPUHandle(index), index);
			}
		}
	);
}

void DxDescriptorTableBuilder::AddUAV(
	ID3D12Resource*							resource,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,
	ID3D12Resource*							counterRes,
	DxDescriptorHandles*					outputHandle
)
{
	bool							 hasDesc = (desc != nullptr);
	D3D12_UNORDERED_ACCESS_VIEW_DESC descCopy = hasDesc ? *desc : D3D12_UNORDERED_ACCESS_VIEW_DESC{};

	m_creators.emplace_back(
		[=](ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t index)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap.GetCPUHandle(index);

			device->CreateUnorderedAccessView(resource, counterRes, hasDesc ? &descCopy : nullptr, cpuHandle);

			if (outputHandle)
			{
				*outputHandle = DxDescriptorHandles(cpuHandle, heap.GetGPUHandle(index), index);
			}
		}
	);
}

void DxDescriptorTableBuilder::AddCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, DxDescriptorHandles* outputHandle)
{
	if (desc)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC descCopy = *desc;
		m_creators.emplace_back(
			[=](ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t index)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap.GetCPUHandle(index);

				device->CreateConstantBufferView(&descCopy, cpuHandle);

				if (outputHandle)
				{
					*outputHandle = DxDescriptorHandles(cpuHandle, heap.GetGPUHandle(index), index);
				}
			}
		);
	}
}

DxDescriptorRange DxDescriptorTableBuilder::Commit(ID3D12Device* device, DxDescriptorHeapGlobal& heap)
{
	if (m_creators.empty())
		return {0, 0};

	uint32_t		  count = static_cast<uint32_t>(m_creators.size());
	DxDescriptorRange allocation = heap.ReserveRange(count);

	for (uint32_t i = 0; i < count; ++i)
	{
		uint32_t currentIndex = allocation.startIndex + i;
		m_creators[i](device, heap, currentIndex);
	}
	m_creators.clear();

	return allocation;
}


void DxDescriptorHeapGlobal::Initialize()
{
	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] WARNING: Initialize() called without parameters. Call Initialize(device, "
				  "count) instead.\n");
}

void DxDescriptorHeapGlobal::Initialize(ID3D12Device* device, uint32_t maxDescriptorCount)
{
	m_capacity = maxDescriptorCount;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = maxDescriptorCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		.NodeMask = 0
	};

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
	m_descriptorIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_baseCPU = m_heap->GetCPUDescriptorHandleForHeapStart();
	m_baseGPU = m_heap->GetGPUDescriptorHandleForHeapStart();

#ifdef _DEBUG
	m_liveMarkers.resize(m_capacity, 0);
#endif

	DxUtils::SetDebugName(m_heap.Get(), "BindlessHeap_CBV_SRV_UAV");

	GRAPHICS_LOG_FMT(
		"[DxDescriptorHeapGlobal] Bindless Heap created: Capacity={}, IncrementSize={} bytes\n", maxDescriptorCount,
		m_descriptorIncrementSize
	);
}

void DxDescriptorHeapGlobal::Release()
{
	const auto	used = GetAllocatedCount();
	const float usage = m_capacity > 0 ? (used * 100.0f / m_capacity) : 0.0f;

	if (used > 0)
	{
		GRAPHICS_LOG_FMT(
			"[DxDescriptorHeapGlobal] WARNING: Release() called with {} descriptors still allocated!\n", used
		);
	}

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] Destroyed: Used={}/{}, Usage={:.1f}%\n", used, m_capacity, usage);

	m_heap.Reset();
	m_freeSlots.clear();
	m_freeRanges.clear();
	m_allocIndex = 0;
	m_capacity = 0;

#ifdef _DEBUG
	m_liveMarkers.clear();
#endif
}

DxDescriptorHandles DxDescriptorHeapGlobal::Allocate()
{
	uint32_t idx;
	if (!m_freeSlots.empty())
	{
		idx = m_freeSlots.back();
		m_freeSlots.pop_back();
	}
	else
	{
		idx = AllocateOneFromFreeRanges();
		if (idx == ~0u)
		{
			if (m_allocIndex >= m_capacity)
			{
				GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] ERROR: Heap full! ({}/{})\n", m_allocIndex, m_capacity);
				std::abort();
			}
			idx = m_allocIndex++;
		}
	}

#ifdef _DEBUG
	if (idx < m_liveMarkers.size() && m_liveMarkers[idx] == 1)
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] ERROR: Double-allocate index {}\n", idx);
		std::abort();
	}
	if (idx >= m_liveMarkers.size())
		m_liveMarkers.resize(m_capacity, 0);
	m_liveMarkers[idx] = 1;
#endif

	return {GetCPUHandle(idx), GetGPUHandle(idx), idx};
}

void DxDescriptorHeapGlobal::FreeImmediate(uint32_t index)
{
	if (index == kInvalidIndex)
	{
		return;
	}

	if (index >= m_capacity)
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] ERROR: Invalid index {} (capacity: {})\n", index, m_capacity);
		return;
	}

#ifdef _DEBUG
	if (index >= m_liveMarkers.size() || m_liveMarkers[index] == 0)
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] ERROR: Double-free index {}\n", index);
		std::abort();
	}
	m_liveMarkers[index] = 0;
#endif

	m_freeSlots.push_back(index);

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] Freed slot: Index={} (available: {})\n", index, m_freeSlots.size());
}

void DxDescriptorHeapGlobal::Free(uint32_t index, const FenceHandle& fenceHandle, std::string_view debugName)
{
	if (index == kInvalidIndex)
	{
		return;
	}
	auto& gc = GLOBAL(DxGarbageCollectorGlobal);
	gc.DeferDescriptorFree(this, index, fenceHandle, debugName);
}

DxDescriptorRange DxDescriptorHeapGlobal::ReserveRange(uint32_t count)
{
	if (count == 0)
		return {0, 0};
	if (auto fromFR = ReserveRangeFromFreeRanges(count); fromFR.count == count)
	{
		GRAPHICS_LOG_FMT(
			"[DxDescriptorHeapGlobal] Reserved range from freeRanges: StartIndex={}, Count={}\n", fromFR.startIndex,
			count
		);
		return fromFR;
	}

	SweepFreeSlotsIntoRanges();
	if (auto fromFR2 = ReserveRangeFromFreeRanges(count); fromFR2.count == count)
	{
		GRAPHICS_LOG_FMT(
			"[DxDescriptorHeapGlobal] Reserved range after sweep: StartIndex={}, Count={}\n", fromFR2.startIndex, count
		);
		return fromFR2;
	}

	if (m_allocIndex + count > m_capacity)
	{
		GRAPHICS_LOG_FMT(
			"[DxDescriptorHeapGlobal] Not enough space for range! Required={}, Available={}\n", count,
			m_capacity - m_allocIndex
		);
		std::abort();
	}

	uint32_t startIndex = m_allocIndex;
	m_allocIndex += count;

#ifdef _DEBUG
	if (startIndex + count > m_liveMarkers.size())
		m_liveMarkers.resize(m_capacity, 0);
	for (uint32_t i = startIndex; i < startIndex + count; ++i)
		m_liveMarkers[i] = 1;
#endif

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] Reserved range: StartIndex={}, Count={}\n", startIndex, count);

	return {startIndex, count};
}

void DxDescriptorHeapGlobal::FreeRangeImmediate(uint32_t startIndex, uint32_t count)
{
	if (count == 0)
		return;
	const uint32_t end = startIndex + count;
	if (end > m_capacity)
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] ERROR: FreeRangeImmediate out of range: [{} , {})\n", startIndex, end);
		return;
	}

#ifdef _DEBUG
	for (uint32_t i = startIndex; i < end; ++i)
	{
		if (i >= m_liveMarkers.size() || m_liveMarkers[i] == 0)
		{
			GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] ERROR: Freeing non-allocated m_index {} in range\n", i);
			std::abort();
		}
		m_liveMarkers[i] = 0;
	}
#endif

	InsertFreeRangeAndMerge(startIndex, end);

	GRAPHICS_LOG_FMT(
		"[DxDescriptorHeapGlobal] Freed range: Start={}, Count={}, freeRanges={}\n", startIndex, count,
		m_freeRanges.size()
	);
}

void DxDescriptorHeapGlobal::FreeRange(
	uint32_t startIndex, uint32_t count, const FenceHandle& fenceHandle, std::string_view debugName
)
{
	auto& gc = GLOBAL(DxGarbageCollectorGlobal);
	gc.DeferRelease([this, startIndex, count] { this->FreeRangeImmediate(startIndex, count); }, fenceHandle, debugName);
}

DxDescriptorHandles DxDescriptorHeapGlobal::CreateSRV(
	ID3D12Device* device, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
)
{
	auto handles = Allocate();
	device->CreateShaderResourceView(resource, desc, handles.GetCPUHandle());

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] SRV created: GlobalIndex={}\n", handles.GetIndex());

	return handles;
}

DxDescriptorHandles DxDescriptorHeapGlobal::CreateUAV(
	ID3D12Device* device, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc
)
{
	auto handles = Allocate();
	device->CreateUnorderedAccessView(resource, nullptr, desc, handles.GetCPUHandle());

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] UAV created: GlobalIndex={}\n", handles.GetIndex());

	return handles;
}

DxDescriptorHandles DxDescriptorHeapGlobal::CreateCBV(ID3D12Device* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc)
{
	auto handles = Allocate();
	device->CreateConstantBufferView(desc, handles.GetCPUHandle());

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] CBV created: GlobalIndex={}\n", handles.GetIndex());

	return handles;
}

void DxDescriptorHeapGlobal::CreateSRVAt(
	ID3D12Device* device, uint32_t globalIndex, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
)
{
	if (globalIndex >= m_allocIndex)
	{
		GRAPHICS_LOG_FMT(
			"[DxDescriptorHeapGlobal] ERROR: Invalid m_index {} (current allocIndex: {})\n", globalIndex, m_allocIndex
		);
		std::abort();
	}

#ifdef _DEBUG
	if (globalIndex >= m_liveMarkers.size() || m_liveMarkers[globalIndex] == 0)
	{
		GRAPHICS_LOG_FMT(
			"[DxDescriptorHeapGlobal] ERROR: Index {} is not allocated (Use-After-Free detected)\n", globalIndex
		);
		std::abort();
	}
#endif

	auto cpuHandle = GetCPUHandle(globalIndex);
	device->CreateShaderResourceView(resource, desc, cpuHandle);
}

DxDescriptorRange DxDescriptorHeapGlobal::CreateSRVRange(
	ID3D12Device* device, const std::vector<ID3D12Resource*>& resources, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc
)
{
	uint32_t count = static_cast<uint32_t>(resources.size());

	auto range = ReserveRange(count);

	for (uint32_t i = 0; i < count; ++i)
	{
		auto cpuHandle = GetCPUHandle(range.startIndex + i);
		device->CreateShaderResourceView(resources[i], desc, cpuHandle);
	}

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] SRV Range created: Start={}, Count={}\n", range.startIndex, count);

	return range;
}

DxDescriptorRange DxDescriptorHeapGlobal::CreateSRVRange(
	ID3D12Device*										device,
	const std::vector<ID3D12Resource*>&					resources,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>& descs
)
{
	if (resources.size() != descs.size())
	{
		GRAPHICS_LOG_FMT(
			"[DxDescriptorHeapGlobal] ERROR: Resource count ({}) != Desc count ({})\n", resources.size(), descs.size()
		);
		std::abort();
	}

	const uint32_t count = static_cast<uint32_t>(resources.size());
	auto		   range = ReserveRange(count);

	for (uint32_t i = 0; i < count; ++i)
	{
		auto cpuHandle = GetCPUHandle(range.startIndex + i);
		device->CreateShaderResourceView(resources[i], &descs[i], cpuHandle);
	}

	GRAPHICS_LOG_FMT(
		"[DxDescriptorHeapGlobal] SRV Range (individual descs) created: StartIndex={}, Count={}\n", range.startIndex,
		count
	);

	return range;
}

uint32_t DxDescriptorHeapGlobal::GetAllocatedCount() const
{
	uint64_t freed = m_freeSlots.size();
	for (const auto& r : m_freeRanges)
		freed += (r.end - r.begin);
	uint64_t total = m_allocIndex;
	uint64_t used = total - freed;
	return static_cast<uint32_t>(used);
}

D3D12_CPU_DESCRIPTOR_HANDLE DxDescriptorHeapGlobal::GetCPUHandle(uint32_t index) const
{
	if (index >= m_capacity)
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] GetCPUHandle index out of range: {} >= {}\n", index, m_capacity);
		std::abort();
	}

	return DxUtils::OffsetHandle(m_baseCPU, index, m_descriptorIncrementSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DxDescriptorHeapGlobal::GetGPUHandle(uint32_t index) const
{
	if (index >= m_capacity)
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] GetGPUHandle index out of range: {} >= {}\n", index, m_capacity);
		std::abort();
	}

	return DxUtils::OffsetHandle(m_baseGPU, index, m_descriptorIncrementSize);
}

void DxDescriptorHeapGlobal::Reset()
{
#ifdef _DEBUG
	const auto used = GetAllocatedCount();
	if (used > 0)
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] WARNING: Reset() called with {} descriptors still allocated!\n", used);
	}
#endif
	m_allocIndex = 0;
	m_freeSlots.clear();
	m_freeRanges.clear();
#ifdef _DEBUG
	std::fill(m_liveMarkers.begin(), m_liveMarkers.end(), 0);
#endif
	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] Reset: All descriptors cleared\n");
}

void DxDescriptorHeapGlobal::LogStats() const
{
	const auto	used = GetAllocatedCount();
	const float usage = m_capacity > 0 ? (used * 100.0f / m_capacity) : 0.0f;

	GRAPHICS_LOG_FMT(
		"[DxDescriptorHeapGlobal] Stats: Used={}/{} ({:.1f}%), IncrSize={}, freeSlots={}, freeRanges={}\n", used,
		m_capacity, usage, m_descriptorIncrementSize, m_freeSlots.size(), m_freeRanges.size()
	);
}

uint32_t DxDescriptorHeapGlobal::AllocateOneFromFreeRanges()
{
	for (auto it = m_freeRanges.begin(); it != m_freeRanges.end(); ++it)
	{
		uint32_t avail = it->end - it->begin;
		if (avail >= 1)
		{
			uint32_t idx = it->begin++;
			if (it->begin == it->end)
				m_freeRanges.erase(it);
			return idx;
		}
	}
	return ~0u;
}

DxDescriptorRange DxDescriptorHeapGlobal::ReserveRangeFromFreeRanges(uint32_t n)
{
	auto	 bestIt = m_freeRanges.end();
	uint32_t minWaste = ~0u;

	for (auto it = m_freeRanges.begin(); it != m_freeRanges.end(); ++it)
	{
		uint32_t avail = it->end - it->begin;
		if (avail < n)
			continue;

		uint32_t waste = avail - n;
		if (waste < minWaste)
		{
			minWaste = waste;
			bestIt = it;
		}
	}

	if (bestIt != m_freeRanges.end())
	{
		uint32_t start = bestIt->begin;
		bestIt->begin += n;
		if (bestIt->begin == bestIt->end)
			m_freeRanges.erase(bestIt);
		return {start, n};
	}

	return {0, 0};
}

void DxDescriptorHeapGlobal::InsertFreeRangeAndMerge(uint32_t begin, uint32_t end)
{
#ifdef _DEBUG
	if (!(begin < end && end <= m_capacity))
	{
		GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] ASSERT: InsertFreeRange invalid range [{}, {})\n", begin, end);
		assert(false);
	}
#endif

	auto pos = std::lower_bound(
		m_freeRanges.begin(), m_freeRanges.end(), FreeRangeEntry{begin, end},
		[](const FreeRangeEntry& a, const FreeRangeEntry& b) { return a.begin < b.begin; }
	);

	if (pos != m_freeRanges.begin())
	{
		auto prev = pos - 1;
		if (prev->end == begin)
		{
			begin = prev->begin;
			pos = m_freeRanges.erase(prev);
		}
	}

	if (pos != m_freeRanges.end() && end == pos->begin)
	{
		end = pos->end;
		pos = m_freeRanges.erase(pos);
	}
	m_freeRanges.insert(pos, FreeRangeEntry{begin, end});

#ifdef _DEBUG
	for (size_t i = 1; i < m_freeRanges.size(); ++i)
	{
		assert(m_freeRanges[i - 1].end <= m_freeRanges[i].begin && "freeRanges must be non-overlapping and sorted");
	}
#endif
}

void DxDescriptorHeapGlobal::SweepFreeSlotsIntoRanges()
{
	if (m_freeSlots.empty())
		return;

	std::sort(m_freeSlots.begin(), m_freeSlots.end());
	m_freeSlots.erase(std::unique(m_freeSlots.begin(), m_freeSlots.end()), m_freeSlots.end());

	uint32_t b = m_freeSlots[0], e = b + 1;
	for (size_t i = 1; i < m_freeSlots.size(); ++i)
	{
		uint32_t x = m_freeSlots[i];
		if (x == e)
		{
			++e;
		}
		else
		{
			InsertFreeRangeAndMerge(b, e);
			b = x;
			e = x + 1;
		}
	}
	InsertFreeRangeAndMerge(b, e);
	m_freeSlots.clear();

	GRAPHICS_LOG_FMT("[DxDescriptorHeapGlobal] Swept free slots into ranges: {} ranges\n", m_freeRanges.size());
}
