#include "stdafxClientFramework.h"
#include "DxCommandContextPool.h"

#include <DxCommandQueueGlobal.h>

DxCommandContextPool::FrameEntry::FrameEntry(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
	: context(device, type)
{
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

DxCommandContextPool::DxCommandContextPool(	ID3D12Device* device,
											IDxGraphicsCommandQueueGlobal& queue,
											uint32_t numFrames)
	: m_queue(queue)
{
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_entries.reserve(numFrames);

	for (uint32_t i = 0; i < numFrames; ++i)
		m_entries.emplace_back(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

DxCommandContextPool::~DxCommandContextPool()
{
	CloseHandle(m_fenceEvent);
}

void DxCommandContextPool::AdvanceFrame()
{
	const uint32_t prevIndex = m_frameIndex;
	m_frameIndex = (m_frameIndex + 1) % m_entries.size();

	// GPU-level synchronization
	const FrameEntry& prevFrame = m_entries[prevIndex];
	// Wait for the GPU to finish processing the previous frame's commands
	m_queue.Wait(prevFrame.fence.Get(), prevFrame.fenceValue);

	WaitForGpu(prevIndex);

	m_entries[m_frameIndex].context.Reset();
}

void DxCommandContextPool::WaitForGpu(uint32_t frameIndex)
{
	const FrameEntry& frame = m_entries[frameIndex];
	if (frame.fence->GetCompletedValue() < frame.fenceValue)
	{
		ThrowIfFailed(frame.fence->SetEventOnCompletion(frame.fenceValue, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

DxCommandContext& DxCommandContextPool::GetCurrentContext()
{
	return m_entries[m_frameIndex].context;
}

void DxCommandContextPool::SignalCurrentFrame()
{
	FrameEntry& frame = m_entries[m_frameIndex];

	frame.context.Close();
	m_queue.ExecuteCommandList(frame.context.CommandList());
	m_queue.Signal(frame.fence.Get(), ++frame.fenceValue);
}
