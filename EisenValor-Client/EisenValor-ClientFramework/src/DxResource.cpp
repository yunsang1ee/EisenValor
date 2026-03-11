#include "stdafxClientFramework.h"
#include "DxResource.h"
#include "DxUtils.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxCommandQueueGlobal.h"
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
		DEBUG_LOG_FMT("[DxResource] GetGPUAddress called on non-buffer resource: {}\n", m_name);
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
	}
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
	if (m_resource)
	{
		ReleaseResource();
	}

	HRESULT hr = device->CreateCommittedResource(
		&heapProps, heapFlags, &resourceDesc, initialState, clearValue, IID_PPV_ARGS(&m_resource)
	);
	if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_HUNG)
		{
			GLOBAL(DxDeviceGlobal).GetMonitor().DumpDred(device);
		}
		ThrowIfFailed(hr);
	}

	if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_sizeInBytes = resourceDesc.Width;
	}
	else
	{
		D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &resourceDesc);
		m_sizeInBytes = allocInfo.SizeInBytes;
	}

	m_currentState = initialState;

	if (!m_name.empty())
	{
		DxUtils::SetDebugName(m_resource.Get(), m_name);
	}

	//DEBUG_LOG_FMT(
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

	// TODO: 멀티큐 이후에 수정 필요
	uint64_t fenceValue = 0;
	switch (m_lastUsedQueue)
	{
	case EQueueType::Graphics:
		fenceValue = GLOBAL(DxGfxCommandQueueGlobal).GetCurrentFenceValue();
		break;
		// case EQueueType::Compute:
		//	fenceValue = GLOBAL(DxComputeCommandQueueGlobal).GetCurrentFenceValue();
		//	break;
		// case EQueueType::Copy:
		//	fenceValue = GLOBAL(DxCopyCommandQueueGlobal).GetCurrentFenceValue();
		//	break;
	}

	FenceHandle currentFence(m_lastUsedQueue, fenceValue + 3);
	gc.DeferResourceRelease(std::move(m_resource), currentFence, m_name);

	if (!m_name.empty())
	{
		DEBUG_LOG_FMT("[DxResource] Resource queued for GC: {} (Fence={})\n", m_name, currentFence.value);
	}
}
