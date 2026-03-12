#pragma once
#include "Singleton.h"
#include "DxCommon.h"

class DxGfxCommandQueueGlobal : public Singleton<DxGfxCommandQueueGlobal>
{
private:
	friend class Singleton<DxGfxCommandQueueGlobal>;

	DxGfxCommandQueueGlobal() = default;
	~DxGfxCommandQueueGlobal() override;

public:
	void Initialize(ID3D12Device* device);
	void Release();

	void ExecuteCommandList(ID3D12CommandList* commandList);
	void Signal(ID3D12Fence* fence, uint64_t fenceValue);
	void Wait(ID3D12Fence* fence, uint64_t fenceValue);
	bool WaitForIdle(uint32_t timeoutMs = INFINITE);

	ID3D12CommandQueue* GetQueue() const { return m_commandQueue.Get(); }
	uint64_t			GetCompletedFenceValue() const { return m_fence->GetCompletedValue(); }
	uint64_t			GetCurrentFenceValue() const { return m_fenceValue; }
	ID3D12Fence*		GetFence() const { return m_fence.Get(); }

	uint64_t SignalFence()
	{
		++m_fenceValue;
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
		return m_fenceValue;
	}

private:
	ComPtr<ID3D12CommandQueue> m_commandQueue;

	ComPtr<ID3D12Fence> m_fence;
	uint64_t			m_fenceValue = 0;
	HANDLE				m_fenceEvent = nullptr;
};