#include "stdafxClientFramework.h"
#include "DxCommandQueueGlobal.h"
#include <DxDeviceGlobal.h>


void DxGraphicsCommandQueueGlobal::Initialize(ID3D12Device* device)
{
	assert(device && "[DxCommandQueue] device is null");
    m_device = device;

	D3D12_COMMAND_QUEUE_DESC desc = {
	    .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
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
    static ComPtr<ID3D12Fence> s_idleFence;
    static uint64_t            s_idleValue = 0;
    static HANDLE              s_event = nullptr;

    if (!s_idleFence)
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&s_idleFence)));
        s_event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(s_event && "Failed to create fence event");
    }

    const uint64_t waitValue = ++s_idleValue;
    ThrowIfFailed(m_commandQueue->Signal(s_idleFence.Get(), waitValue));

    if (s_idleFence->GetCompletedValue() < waitValue)
    {
        ThrowIfFailed(s_idleFence->SetEventOnCompletion(waitValue, s_event));
        ::WaitForSingleObject(s_event, INFINITE);
    }
}