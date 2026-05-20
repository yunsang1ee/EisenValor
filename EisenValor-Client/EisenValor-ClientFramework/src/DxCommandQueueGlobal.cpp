#include "stdafxClientFramework.h"
#include "DxCommandQueueGlobal.h"
#include "DxDeviceGlobal.h"

DxGfxCommandQueueGlobal::~DxGfxCommandQueueGlobal()
{
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
	GRAPHICS_LOG_FMT("[DxGfxCommandQueueGlobal] Destroyed DxGfxCommandQueueGlobal.\n");
}

void DxGfxCommandQueueGlobal::Initialize(ID3D12Device* device)
{
	assert(device && "[DxGfxCommandQueueGlobal] device is null");

	D3D12_COMMAND_QUEUE_DESC desc = {
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
	m_commandQueue->SetName(L"GfxQueue");

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence->SetName(L"GfxQueueFence");

	m_fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(m_fenceEvent && "Failed to create fence event");

	GRAPHICS_LOG_FMT("[DxGfxCommandQueueGlobal] Initialized DxGfxCommandQueueGlobal.\n");
}

void DxGfxCommandQueueGlobal::Release()
{
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
	m_commandQueue.Reset();
	m_fence.Reset();
	m_fenceValue = 0;
}

void DxGfxCommandQueueGlobal::ExecuteCommandList(ID3D12CommandList* commandList)
{
	if (!commandList)
	{
		GRAPHICS_LOG_FMT("[DxGfxCommandQueueGlobal] ExecuteCommandList: commandList is null\n");
		return;
	}
	m_commandQueue->ExecuteCommandLists(1, &commandList);
}

void DxGfxCommandQueueGlobal::Signal(ID3D12Fence* fence, uint64_t fenceValue)
{
	assert(fence && "[DxGfxCommandQueueGlobal] fence is null");
	ThrowIfFailed(m_commandQueue->Signal(fence, fenceValue));
}

void DxGfxCommandQueueGlobal::Wait(ID3D12Fence* fence, uint64_t fenceValue)
{
	assert(fence && "[DxGfxCommandQueueGlobal] fence is null");
	ThrowIfFailed(m_commandQueue->Wait(fence, fenceValue));
}

bool DxGfxCommandQueueGlobal::WaitForIdle(uint32_t timeoutMs)
{
	const uint64_t waitValue = SignalFence();

	if (waitValue > 0 && m_fence->GetCompletedValue() < waitValue)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(waitValue, m_fenceEvent));
		if (::WaitForSingleObject(m_fenceEvent, timeoutMs) == WAIT_TIMEOUT)
		{
			GRAPHICS_LOG_FMT("[DxGfxCommandQueueGlobal] WaitForIdle timed out (Fence={})\n", waitValue);
			return false;
		}
	}

	GRAPHICS_LOG_FMT("[DxGfxCommandQueueGlobal] WaitForIdle completed (Fence={})\n", waitValue);
	return true;
}