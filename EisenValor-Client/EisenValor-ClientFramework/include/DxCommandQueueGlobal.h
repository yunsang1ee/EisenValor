#pragma once

#pragma region Interfaces
class IDxGfxCommandQueueGlobal : public IGlobal
{
public:
	virtual void Initialize(ID3D12Device* device) = 0;
	virtual void Release() = 0;

	virtual void ExecuteCommandList(ID3D12CommandList* commandList) = 0;
	virtual void Signal(ID3D12Fence* fence, uint64_t fenceValue) = 0;
	virtual void Wait(ID3D12Fence* fence, uint64_t fenceValue) = 0;
	virtual void WaitForIdle() = 0;

	virtual ID3D12CommandQueue* GetQueue() const = 0;
};

class IDxComputeCommandQueueGlobal : public IGlobal
{
	// This class would be similar to DxGfxCommandQueueGlobal but for compute operations.
};

class IDxCopyCommandQueueGlobal : public IGlobal
{
	// This class would be similar to DxGfxCommandQueueGlobal but for copy operations.
};
#pragma endregion

#pragma region Implementations
class DxGfxCommandQueueGlobal : public GlobalMakerBase<DxGfxCommandQueueGlobal, IDxGfxCommandQueueGlobal>
{
public:
	~DxGfxCommandQueueGlobal();

	void Initialize(ID3D12Device* device) override;
	void Release() override;

	void ExecuteCommandList(ID3D12CommandList* commandList) override;
	void Signal(ID3D12Fence* fence, uint64_t fenceValue) override;
	// Wait for the GPU to finish processing commands
	void Wait(ID3D12Fence* fence, uint64_t fenceValue) override;
	void WaitForIdle() override;

	ID3D12CommandQueue* GetQueue() const override { return m_commandQueue.Get(); }
	uint64_t			GetCompletedFenceValue() const { return m_idleFence->GetCompletedValue(); }
	uint64_t			GetCurrentFenceValue() const { return m_idleValue; }

private:
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ID3D12Device*			   m_device = nullptr;

	ComPtr<ID3D12Fence> m_idleFence;
	uint64_t			m_idleValue = 0;
	HANDLE				m_idleEvent = nullptr;
};

class DxComputeCommandQueueGlobal : public GlobalMakerBase<DxComputeCommandQueueGlobal, IDxComputeCommandQueueGlobal>
{
	// This class would be similar to DxGfxCommandQueueGlobal but for compute operations.
};

class DxCopyCommandQueueGlobal : public GlobalMakerBase<DxCopyCommandQueueGlobal, IDxCopyCommandQueueGlobal>
{
	// This class would be similar to DxGfxCommandQueueGlobal but for copy operations.
};

#pragma endregion