#include "stdafxClientFramework.h"
#include "DxCommandContextPool.h"

void DxCommandContextPool::Initialize(ID3D12Device* device, uint32_t numWorkers, D3D12_COMMAND_LIST_TYPE type)
{
	assert(device && "[DxCommandContextPool] Device is null");
	assert(numWorkers > 0 && numWorkers <= 64 && "[DxCommandContextPool] Invalid worker count");

	m_type = type;
	m_workers.resize(numWorkers);
	m_availableIndices.clear();
	m_availableIndices.reserve(numWorkers);

	for (uint32_t i = 0; i < numWorkers; ++i)
	{
		m_workers[i].context = std::make_unique<DxCommandContext>(device, type);
		m_workers[i].state = EState::Free;
		m_availableIndices.push_back(i);
	}

	DEBUG_LOG_FMT("[DxCommandContextPool] Initialized {} workers (Type: {})\n", numWorkers, static_cast<int>(type));
}

void DxCommandContextPool::ResetAll()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& worker : m_workers)
	{
		worker.context->Reset();
		worker.state = EState::Free;
	}

	m_availableIndices.clear();
	m_availableIndices.reserve(m_workers.size());
	for (uint32_t i = 0; i < m_workers.size(); ++i)
	{
		m_availableIndices.push_back(i);
	}

	DEBUG_LOG_FMT("[DxCommandContextPool] Reset {} workers\n", m_workers.size());
}

DxCommandContext* DxCommandContextPool::Acquire()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_availableIndices.empty())
	{
		DEBUG_LOG_FMT("[DxCommandContextPool] Warning: No available workers\n");
		return nullptr;
	}

	uint32_t workerIndex = m_availableIndices.back();
	m_availableIndices.pop_back();
	auto& worker = m_workers[workerIndex];
	assert(worker.state == EState::Free && "[DxCommandContextPool] Acquiring a non-free worker"); 
	worker.state = EState::Recording;

	return worker.context.get();
}

void DxCommandContextPool::Release(DxCommandContext* context)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (uint32_t i = 0; i < m_workers.size(); ++i)
	{
		auto& worker = m_workers[i];
		if (worker.context.get() == context)
		{
			assert(worker.state == EState::Recording && "[DxCommandContextPool] Releasing non-recording worker");
			if (worker.context->IsRecording())
			{
				worker.context->Close();
			}
			worker.state = EState::Ready;
			return;
		}
	}

	assert(false && "[DxCommandContextPool] Context not found in pool");
}

std::vector<ID3D12CommandList*> DxCommandContextPool::GetAllCommandLists()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	const auto readyCount = std::count_if(
		m_workers.begin(), m_workers.end(), [](const WorkerEntry& w) { return w.state == EState::Ready; }
	);

	std::vector<ID3D12CommandList*> lists;
	lists.reserve(readyCount);

	for (auto& w : m_workers)
	{
		if (w.state == EState::Ready)
		{
			lists.push_back(w.context->CommandList());
		}
	}
	return lists;
}

uint32_t DxCommandContextPool::GetAvailableCount() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return static_cast<uint32_t>(m_availableIndices.size());
}