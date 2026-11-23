#pragma once
#include "DxCommon.h"
#include "DxBuffer.h"
#include "DxUploadHeap.h"
#include <optional>

class DxCommandContext;

class DxFrameResource
{
public:
	DxFrameResource();
	~DxFrameResource();

	DxFrameResource(const DxFrameResource&) = delete;
	DxFrameResource& operator=(const DxFrameResource&) = delete;

	DxFrameResource(DxFrameResource&&) = delete;
	DxFrameResource& operator=(DxFrameResource&&) = delete;

	void Initialize(
		ID3D12Device*			device,
		uint32_t				frameIndex,
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		uint64_t				uploadHeapSizeInBytes = 256 * 1024 * 1024
		/*uint32_t				numWorkers = 0*/ // Multi-thread 시 향후 추가
	);

	void BeginFrame(bool measureGpuTime = false);
	void ExecuteAndSignal(ID3D12CommandQueue* queue);
	void WaitForCompletion();
	bool IsGpuTimingSupported() const { return m_timestampHeap && m_timestampReadback; }
	std::optional<uint64_t> GetCompletedGpuTicks() const { return m_completedGpuTicks; }
	double GetCpuFenceWaitMs() const { return m_cpuFenceWaitMs; }

	DxUploadHeap*	  GetUploadHeap() const { return m_uploadHeap.get(); }
	DxCommandContext* GetMainContext() const { return m_mainContext.get(); }

	// Multi-thread 시 향후 추가
	// DxCommandContext* AcquireWorkerContext() { return nullptr; }
	// void			  ReleaseWorkerContext(DxCommandContext*) {}

	ID3D12Fence*			GetFence() const { return m_fence.Get(); }
	uint64_t				GetFenceValue() const { return m_fenceValue; }
	uint32_t				GetFrameIndex() const { return m_frameIndex; }
	bool					IsInitialized() const { return m_mainContext != nullptr; }
	D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_commandListType; }

private:
	uint32_t				m_frameIndex = 0;
	D3D12_COMMAND_LIST_TYPE m_commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;

	std::unique_ptr<DxCommandContext> m_mainContext;
	std::unique_ptr<DxUploadHeap>	  m_uploadHeap;

	ComPtr<ID3D12Fence> m_fence;
	uint64_t			m_fenceValue = 0;
	HANDLE				m_fenceEvent = nullptr;
	ComPtr<ID3D12QueryHeap> m_timestampHeap;
	ComPtr<ID3D12Resource> m_timestampReadback;
	std::optional<uint64_t> m_completedGpuTicks;
	double m_cpuFenceWaitMs = 0.0;
	bool m_recordGpuTime = false;
	bool m_timestampPending = false;

	// TODO: DxFrameDescriptorAllocator 등
};
