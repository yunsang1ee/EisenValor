#include "stdafxClientFramework.h"
#include "DxResourceAllocationTracker.h"
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

#if ENABLE_GRAPHICS_DEBUG_LOG
struct DxResourceAllocationTracker::TrackedAllocation
{
	std::string				 name;
	uint64_t				 sizeInBytes = 0;
	D3D12_HEAP_TYPE			 heapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
	bool					 pendingRelease = false;
};

struct DxResourceAllocationTracker::SnapshotEntry
{
	ID3D12Resource*			 resource = nullptr;
	std::string				 name;
	uint64_t				 sizeInBytes = 0;
	D3D12_HEAP_TYPE			 heapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
	bool					 pendingRelease = false;
};

struct DxResourceAllocationTracker::State
{
	std::mutex											   allocationMutex;
	std::unordered_map<ID3D12Resource*, TrackedAllocation> allocations;
};

DxResourceAllocationTracker::State& DxResourceAllocationTracker::GetState()
{
	static State state;
	return state;
}

double DxResourceAllocationTracker::ToMiB(uint64_t bytes)
{
	return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

const char* DxResourceAllocationTracker::HeapTypeName(D3D12_HEAP_TYPE heapType)
{
	switch (heapType)
	{
	case D3D12_HEAP_TYPE_DEFAULT:
		return "DEFAULT";
	case D3D12_HEAP_TYPE_UPLOAD:
		return "UPLOAD";
	case D3D12_HEAP_TYPE_READBACK:
		return "READBACK";
	case D3D12_HEAP_TYPE_CUSTOM:
		return "CUSTOM";
	default:
		return "UNKNOWN";
	}
}

const char* DxResourceAllocationTracker::DimensionName(D3D12_RESOURCE_DIMENSION dimension)
{
	switch (dimension)
	{
	case D3D12_RESOURCE_DIMENSION_BUFFER:
		return "BUFFER";
	case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		return "TEXTURE1D";
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		return "TEXTURE2D";
	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		return "TEXTURE3D";
	default:
		return "UNKNOWN";
	}
}
#endif

void DxResourceAllocationTracker::Track(
	ID3D12Resource*			 resource,
	std::string_view		 name,
	uint64_t				 sizeInBytes,
	D3D12_HEAP_TYPE			 heapType,
	D3D12_RESOURCE_DIMENSION dimension
)
{
#if ENABLE_GRAPHICS_DEBUG_LOG
	if (!resource)
	{
		return;
	}

	auto&			state = GetState();
	std::lock_guard lock(state.allocationMutex);
	state.allocations[resource] = TrackedAllocation{
		.name = std::string(name),
		.sizeInBytes = sizeInBytes,
		.heapType = heapType,
		.dimension = dimension,
		.pendingRelease = false,
	};
#else
	(void)resource;
	(void)name;
	(void)sizeInBytes;
	(void)heapType;
	(void)dimension;
#endif
}

void DxResourceAllocationTracker::Rename(ID3D12Resource* resource, std::string_view name)
{
#if ENABLE_GRAPHICS_DEBUG_LOG
	if (!resource)
	{
		return;
	}

	auto&			state = GetState();
	std::lock_guard lock(state.allocationMutex);
	auto			iter = state.allocations.find(resource);
	if (iter != state.allocations.end())
	{
		iter->second.name = std::string(name);
	}
#else
	(void)resource;
	(void)name;
#endif
}

void DxResourceAllocationTracker::MarkPendingRelease(ID3D12Resource* resource)
{
#if ENABLE_GRAPHICS_DEBUG_LOG
	if (!resource)
	{
		return;
	}

	auto&			state = GetState();
	std::lock_guard lock(state.allocationMutex);
	auto			iter = state.allocations.find(resource);
	if (iter != state.allocations.end())
	{
		iter->second.pendingRelease = true;
	}
#else
	(void)resource;
#endif
}

void DxResourceAllocationTracker::Untrack(ID3D12Resource* resource)
{
#if ENABLE_GRAPHICS_DEBUG_LOG
	if (!resource)
	{
		return;
	}

	auto&			state = GetState();
	std::lock_guard lock(state.allocationMutex);
	state.allocations.erase(resource);
#else
	(void)resource;
#endif
}

void DxResourceAllocationTracker::LogStats(IDXGIAdapter4* adapter, std::string_view reason, uint32_t topCount)
{
#if ENABLE_GRAPHICS_DEBUG_LOG
	std::vector<SnapshotEntry> entries;
	{
		auto&			state = GetState();
		std::lock_guard lock(state.allocationMutex);
		entries.reserve(state.allocations.size());
		for (const auto& [resource, allocation] : state.allocations)
		{
			entries.push_back(
				{resource, allocation.name, allocation.sizeInBytes, allocation.heapType, allocation.dimension,
				 allocation.pendingRelease}
			);
		}
	}

	uint64_t liveDefault = 0;
	uint64_t pendingDefault = 0;
	uint64_t liveUpload = 0;
	uint64_t pendingUpload = 0;
	uint64_t liveReadback = 0;
	uint64_t pendingReadback = 0;
	uint32_t liveCount = 0;
	uint32_t pendingCount = 0;

	for (const auto& entry : entries)
	{
		if (entry.heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			if (entry.pendingRelease)
			{
				pendingUpload += entry.sizeInBytes;
			}
			else
			{
				liveUpload += entry.sizeInBytes;
			}
		}
		else if (entry.heapType == D3D12_HEAP_TYPE_READBACK)
		{
			if (entry.pendingRelease)
			{
				pendingReadback += entry.sizeInBytes;
			}
			else
			{
				liveReadback += entry.sizeInBytes;
			}
		}
		else
		{
			if (entry.pendingRelease)
			{
				pendingDefault += entry.sizeInBytes;
			}
			else
			{
				liveDefault += entry.sizeInBytes;
			}
		}

		if (entry.pendingRelease)
		{
			++pendingCount;
		}
		else
		{
			++liveCount;
		}
	}

	DXGI_QUERY_VIDEO_MEMORY_INFO localInfo{};
	DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalInfo{};
	const HRESULT				 localHr =
		   adapter ? adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo) : E_FAIL;
	const HRESULT nonLocalHr =
		adapter ? adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalInfo) : E_FAIL;

	if (SUCCEEDED(localHr))
	{
		const uint64_t trackedLocal = liveDefault + pendingDefault;
		const int64_t  untrackedLocal =
			static_cast<int64_t>(localInfo.CurrentUsage) - static_cast<int64_t>(trackedLocal);
		GRAPHICS_LOG_FMT(
			"[VRAM] reason={} localUsage={:.2f}MiB localBudget={:.2f}MiB trackedDefault={:.2f}MiB "
			"pendingDefault={:.2f}MiB externalOrDriver={:.2f}MiB liveCount={} pendingCount={}\n",
			reason, ToMiB(localInfo.CurrentUsage), ToMiB(localInfo.Budget), ToMiB(liveDefault), ToMiB(pendingDefault),
			static_cast<double>(untrackedLocal) / (1024.0 * 1024.0), liveCount, pendingCount
		);
	}
	else
	{
		GRAPHICS_LOG_FMT("[VRAM] reason={} QueryVideoMemoryInfo(LOCAL) failed: 0x{:08X}\n", reason, (uint32_t)localHr);
	}

	if (SUCCEEDED(nonLocalHr))
	{
		GRAPHICS_LOG_FMT(
			"[VRAM] reason={} nonLocalUsage={:.2f}MiB nonLocalBudget={:.2f}MiB trackedUpload={:.2f}MiB "
			"pendingUpload={:.2f}MiB trackedReadback={:.2f}MiB pendingReadback={:.2f}MiB\n",
			reason, ToMiB(nonLocalInfo.CurrentUsage), ToMiB(nonLocalInfo.Budget), ToMiB(liveUpload),
			ToMiB(pendingUpload), ToMiB(liveReadback), ToMiB(pendingReadback)
		);
	}

	std::sort(
		entries.begin(), entries.end(),
		[](const SnapshotEntry& lhs, const SnapshotEntry& rhs) { return lhs.sizeInBytes > rhs.sizeInBytes; }
	);

	const uint32_t count = std::min<uint32_t>(topCount, static_cast<uint32_t>(entries.size()));
	for (uint32_t i = 0; i < count; ++i)
	{
		const auto& entry = entries[i];
		GRAPHICS_LOG_FMT(
			"[VRAM.Top] rank={} size={:.2f}MiB heap={} dim={} pending={} name='{}'\n", i + 1, ToMiB(entry.sizeInBytes),
			HeapTypeName(entry.heapType), DimensionName(entry.dimension), entry.pendingRelease ? 1 : 0,
			entry.name.empty() ? "(unnamed)" : entry.name
		);
	}
#else
	(void)adapter;
	(void)reason;
	(void)topCount;
#endif
}
