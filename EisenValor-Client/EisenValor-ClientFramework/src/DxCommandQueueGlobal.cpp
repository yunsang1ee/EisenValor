#include "stdafxClientFramework.h"
#include "DxCommandQueueGlobal.h"
#include "DxDeviceGlobal.h"

DxGfxCommandQueueGlobal::~DxGfxCommandQueueGlobal()
{
	if (m_idleEvent)
	{
		CloseHandle(m_idleEvent);
		m_idleEvent = nullptr;
	}
	DEBUG_LOG_FMT("[DxGfxCommandQueueGlobal] Destroyed DxGfxCommandQueueGlobal.\n");
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

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_idleFence)));
	m_idleEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(m_idleEvent && "Failed to create fence event");

	DEBUG_LOG_FMT("[DxGfxCommandQueueGlobal] Initialized DxGfxCommandQueueGlobal.\n");
}

void DxGfxCommandQueueGlobal::Release()
{
	if (m_idleEvent)
	{
		CloseHandle(m_idleEvent);
		m_idleEvent = nullptr;
	}
	m_commandQueue.Reset();
	m_idleFence.Reset();
	m_idleValue = 0;
}

void DxGfxCommandQueueGlobal::ExecuteCommandList(ID3D12CommandList* commandList)
{
	if (!commandList)
	{
		DEBUG_LOG_FMT("[DxGfxCommandQueueGlobal] ExecuteCommandList: commandList is null\n");
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

void DxGfxCommandQueueGlobal::WaitForIdle()
{
	const uint64_t waitValue = ++m_idleValue;
	ThrowIfFailed(m_commandQueue->Signal(m_idleFence.Get(), waitValue));

	if (m_idleFence->GetCompletedValue() < waitValue)
	{
		ThrowIfFailed(m_idleFence->SetEventOnCompletion(waitValue, m_idleEvent));
		::WaitForSingleObject(m_idleEvent, INFINITE);
	}
}