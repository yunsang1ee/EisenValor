#pragma once
#include "DxCommon.h"

class DxResourceAllocationTracker
{
public:
	static void Track(
		ID3D12Resource*			 resource,
		std::string_view		 name,
		uint64_t				 sizeInBytes,
		D3D12_HEAP_TYPE			 heapType,
		D3D12_RESOURCE_DIMENSION dimension
	);
	static void Rename(ID3D12Resource* resource, std::string_view name);
	static void MarkPendingRelease(ID3D12Resource* resource);
	static void Untrack(ID3D12Resource* resource);
	static void LogStats(IDXGIAdapter4* adapter, std::string_view reason, uint32_t topCount = 16);

private:
	struct TrackedAllocation;
	struct SnapshotEntry;
	struct State;

	static State&	   GetState();
	static double	   ToMiB(uint64_t bytes);
	static const char* HeapTypeName(D3D12_HEAP_TYPE heapType);
	static const char* DimensionName(D3D12_RESOURCE_DIMENSION dimension);
};
