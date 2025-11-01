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
	void WaitForIdle();

	ID3D12CommandQueue* GetQueue() const { return m_commandQueue.Get(); }
	uint64_t			GetCompletedFenceValue() const { return m_idleFence->GetCompletedValue(); }
	uint64_t			GetCurrentFenceValue() const { return m_idleValue; }

private:
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12Fence>		   m_idleFence;
	uint64_t				   m_idleValue = 0;
	HANDLE					   m_idleEvent = nullptr;
};