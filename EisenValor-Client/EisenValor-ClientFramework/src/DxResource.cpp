#include "stdafxClientFramework.h"
#include "DxResource.h"
#include "DxResourceAllocationTracker.h"
#include "DxUtils.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxDeviceGlobal.h"

DxResource::~DxResource()
{
	ReleaseResource();
}

DxResource::DxResource(DxResource&& other) noexcept
	: m_resource(std::move(other.m_resource)), m_currentState(other.m_currentState), m_sizeInBytes(other.m_sizeInBytes),
	  m_name(std::move(other.m_name))
{
	other.m_currentState = D3D12_RESOURCE_STATE_COMMON;
	other.m_sizeInBytes = 0;
	other.m_lastUsedQueue = EQueueType::Graphics;
}

DxResource& DxResource::operator=(DxResource&& other) noexcept
{
	if (this != &other)
	{
		ReleaseResource();

		m_lastUsedQueue = other.m_lastUsedQueue;
		m_resource = std::move(other.m_resource);
		m_currentState = other.m_currentState;
		m_sizeInBytes = other.m_sizeInBytes;
		m_name = std::move(other.m_name);

		other.m_currentState = D3D12_RESOURCE_STATE_COMMON;
		other.m_sizeInBytes = 0;
	}
	return *this;
}

D3D12_GPU_VIRTUAL_ADDRESS DxResource::GetGPUAddress(uint64_t offset) const
{
	if (!m_resource)
		return 0;

	auto desc = m_resource->GetDesc();
	if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		GRAPHICS_LOG_FMT("[DxResource] GetGPUAddress called on non-buffer resource: {}\n", m_name);
		return 0;
	}

	return m_resource->GetGPUVirtualAddress() + offset;
}

void DxResource::SetName(std::string_view name)
{
	m_name = name;
	if (m_resource)
	{
		DxUtils::SetDebugName(m_resource.Get(), name);
		DxResourceAllocationTracker::Rename(m_resource.Get(), name);
	}
}

void DxResource::AdoptResource(
	ID3D12Device*		   device,
	ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES  initialState,
	D3D12_HEAP_TYPE		   heapType,
	std::string_view	   name
)
{
	assert(device && resource);

	const std::string resourceName(name);
	if (m_resource.Get() != resource.Get())
	{
		ReleaseResource();
	}

	m_resource = std::move(resource);
	m_currentState = initialState;
	m_name = resourceName;

	const D3D12_RESOURCE_DESC			 resourceDesc = m_resource->GetDesc();
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = device->GetResourceAllocationInfo(0, 1, &resourceDesc);
	m_sizeInBytes =
		resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ? resourceDesc.Width : allocationInfo.SizeInBytes;

	if (!m_name.empty())
	{
		DxUtils::SetDebugName(m_resource.Get(), m_name);
	}

	DxResourceAllocationTracker::Track(
		m_resource.Get(), m_name, allocationInfo.SizeInBytes, heapType, resourceDesc.Dimension
	);
}

void DxResource::InitializeResource(
	ID3D12Device*				 device,
	const D3D12_HEAP_PROPERTIES& heapProps,
	D3D12_HEAP_FLAGS			 heapFlags,
	const D3D12_RESOURCE_DESC&	 resourceDesc,
	D3D12_RESOURCE_STATES		 initialState,
	const D3D12_CLEAR_VALUE*	 clearValue
)
{
	ComPtr<ID3D12Resource> resource;
	HRESULT				   hr = device->CreateCommittedResource(
		   &heapProps, heapFlags, &resourceDesc, initialState, clearValue, IID_PPV_ARGS(&resource)
	   );
	if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_HUNG)
		{
			GLOBAL(DxDeviceGlobal).GetMonitor().DumpDred(device);
		}
		ThrowIfFailed(hr);
	}

	AdoptResource(device, std::move(resource), initialState, heapProps.Type, m_name);

	//GRAPHICS_LOG_FMT(
	//	"[DxResource] Resource created: {}, {} bytes, State: 0x{:X}\n", m_name, m_sizeInBytes,
	//	static_cast<uint32_t>(m_currentState)
	//);
}

void DxResource::ReleaseResource()
{
	if (!m_resource)
	{
		return;
	}

	auto& gc = GLOBAL(DxGarbageCollectorGlobal);

	ID3D12Resource* releasedResource = m_resource.Get();
	DxResourceAllocationTracker::MarkPendingRelease(releasedResource);
	gc.DeferResourceReleaseAfterCurrentFrame(
		std::move(m_resource), m_name, [releasedResource] { DxResourceAllocationTracker::Untrack(releasedResource); }
	);

	if (!m_name.empty())
	{
		GRAPHICS_LOG_FMT("[DxResource] Resource queued for release after current frame: {}\n", m_name);
	}

	m_sizeInBytes = 0;
	m_currentState = D3D12_RESOURCE_STATE_COMMON;
}
