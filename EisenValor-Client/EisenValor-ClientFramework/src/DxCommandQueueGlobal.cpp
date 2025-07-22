#include "stdafxClientFramework.h"
#include "DxCommandQueueGlobal.h"
#include "DxDeviceGlobal.h"

DxGraphicsCommandQueueGlobal::~DxGraphicsCommandQueueGlobal()
{
	if (m_idleEvent)
	{
		CloseHandle(m_idleEvent);
		m_idleEvent = nullptr;
	}
	DEBUG_LOG_FMT("[DxGraphicsCommandQueueGlobal] Destroyed DxGraphicsCommandQueueGlobal.\n");
}

void DxGraphicsCommandQueueGlobal::Initialize(ID3D12Device* device)
{
	assert(device && "[DxGraphicsCommandQueueGlobal] device is null");
    m_device = device;

	D3D12_COMMAND_QUEUE_DESC desc = {
	    .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    };

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));

	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_idleFence)));
	m_idleEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(m_idleEvent && "Failed to create fence event");

	DEBUG_LOG_FMT("[DxGraphicsCommandQueueGlobal] Initialized DxGraphicsCommandQueueGlobal.\n");
}

void DxGraphicsCommandQueueGlobal::ExecuteCommandList(ID3D12CommandList* commandList)
{
	assert(commandList && "[DxGraphicsCommandQueueGlobal] commandList is null");
	m_commandQueue->ExecuteCommandLists(1, &commandList);
}

void DxGraphicsCommandQueueGlobal::Signal(ID3D12Fence* fence, uint64_t fenceValue)
{
	assert(fence && "[DxGraphicsCommandQueueGlobal] fence is null");
	ThrowIfFailed(m_commandQueue->Signal(fence, fenceValue));
}

void DxGraphicsCommandQueueGlobal::Wait(ID3D12Fence* fence, uint64_t fenceValue)
{
	assert(fence && "[DxGraphicsCommandQueueGlobal] fence is null");
	ThrowIfFailed(m_commandQueue->Wait(fence, fenceValue));
}

void DxGraphicsCommandQueueGlobal::WaitForIdle()
{
    const uint64_t waitValue = ++m_idleValue;
    ThrowIfFailed(m_commandQueue->Signal(m_idleFence.Get(), waitValue));

    if (m_idleFence->GetCompletedValue() < waitValue)
    {
        ThrowIfFailed(m_idleFence->SetEventOnCompletion(waitValue, m_idleEvent));
        ::WaitForSingleObject(m_idleEvent, INFINITE);
    }
}