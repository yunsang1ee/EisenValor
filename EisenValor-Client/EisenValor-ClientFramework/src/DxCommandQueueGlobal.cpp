#include "stdafxClientFramework.h"
#include "DxCommandQueueGlobal.h"
#include "DxDeviceGlobal.h"


void DxGraphicsCommandQueueGlobal::Initialize(ID3D12Device* device)
{
	assert(device && "[DxCommandQueue] device is null");
    m_device = device;

	D3D12_COMMAND_QUEUE_DESC desc = {
	    .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    };

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));

	DEBUG_LOG_FMT("[DxCommandQueue] Initialized CommandQueue.\n");
}

void DxGraphicsCommandQueueGlobal::ExecuteCommandList(ID3D12CommandList* commandList)
{
	assert(commandList && "[DxCommandQueue] commandList is null");
	m_commandQueue->ExecuteCommandLists(1, &commandList);
}

void DxGraphicsCommandQueueGlobal::Signal(ID3D12Fence* fence, uint64_t fenceValue)
{
	assert(fence && "[DxCommandQueue] fence is null");
	ThrowIfFailed(m_commandQueue->Signal(fence, fenceValue));
}

void DxGraphicsCommandQueueGlobal::Wait(ID3D12Fence* fence, uint64_t fenceValue)
{
	assert(fence && "[DxCommandQueue] fence is null");
	ThrowIfFailed(m_commandQueue->Wait(fence, fenceValue));
}

void DxGraphicsCommandQueueGlobal::WaitForIdle()
{
    static ComPtr<ID3D12Fence> idleFence;
    static uint64_t            idleValue = 0;
    static HANDLE              event = nullptr;

    if (!idleFence)
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&idleFence)));
        event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(event && "Failed to create fence event");
    }

    const uint64_t waitValue = ++idleValue;
    ThrowIfFailed(m_commandQueue->Signal(idleFence.Get(), waitValue));

    if (idleFence->GetCompletedValue() < waitValue)
    {
        ThrowIfFailed(idleFence->SetEventOnCompletion(waitValue, event));
        ::WaitForSingleObject(event, INFINITE);
    }
}