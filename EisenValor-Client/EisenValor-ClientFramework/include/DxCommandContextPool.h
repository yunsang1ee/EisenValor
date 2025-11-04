#pragma once
#include <DxCommandContext.h>
#include <mutex>

class DxCommandContextPool
{
public:
	DxCommandContextPool() = default;
	~DxCommandContextPool() = default;

	DxCommandContextPool(const DxCommandContextPool&) = delete;
	DxCommandContextPool& operator=(const DxCommandContextPool&) = delete;

	void Initialize(
		ID3D12Device* device, uint32_t numWorkers, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT
	);

	void ResetAll();

	DxCommandContext* Acquire();

	void Release(DxCommandContext* context);

	std::vector<ID3D12CommandList*> GetAllCommandLists();

	uint32_t GetAvailableCount() const;
	uint32_t GetTotalCount() const { return static_cast<uint32_t>(m_workers.size()); }

private:
	enum class EState : uint8_t
	{
		Free,
		Recording,
		Ready
	};
	struct WorkerEntry
	{
		std::unique_ptr<DxCommandContext> context;
		EState							  state = EState::Free;
	};

	std::vector<WorkerEntry> m_workers;
	std::vector<uint32_t>	 m_availableIndices;
	D3D12_COMMAND_LIST_TYPE	 m_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	mutable std::mutex		 m_mutex;
};
