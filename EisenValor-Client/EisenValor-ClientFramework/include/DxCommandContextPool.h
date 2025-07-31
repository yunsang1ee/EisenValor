#pragma once
#include <DxCommandContext.h>

class DxCommandContextPool
{
public:
	DxCommandContextPool(ID3D12Device* device,
		class IDxGraphicsCommandQueueGlobal& queue,
		uint32_t numFrames = 3,
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	~DxCommandContextPool();

	void AdvanceFrame();
	void WaitForGPU(uint32_t frameIndex);

	DxCommandContext& GetCurrentContext();
	uint32_t GetCurrentFrameIndex() const { return m_frameIndex; }

	void SignalCurrentFrame();

private:
	struct FrameEntry
	{
		DxCommandContext	context;
		ComPtr<ID3D12Fence> fence;
		uint64_t			fenceValue = 0;

		FrameEntry(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, uint32_t frameIndex);
	};

	std::vector<FrameEntry>	m_entries;
	uint32_t				m_frameIndex = 0;
	HANDLE					m_fenceEvent;
	IDxGraphicsCommandQueueGlobal&	m_queue;
	D3D12_COMMAND_LIST_TYPE m_type;
};

	