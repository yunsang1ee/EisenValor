#pragma once
#include "Singleton.h"
#include <queue>
#include <functional>

class DxDescriptorHeapGlobal;

struct CompletedFences
{
	uint64_t graphics = 0;
	uint64_t compute = 0;
	uint64_t copy = 0;

	uint64_t GetCompleted(EQueueType type) const
	{
		switch (type)
		{
		case EQueueType::Graphics:
			return graphics;
		case EQueueType::Compute:
			return compute;
		case EQueueType::Copy:
			return copy;
		default:
			return 0;
		}
	}
};

class DxGarbageCollectorGlobal : public Singleton<DxGarbageCollectorGlobal>
{
private:
	friend class Singleton<DxGarbageCollectorGlobal>;
	DxGarbageCollectorGlobal() = default;
	~DxGarbageCollectorGlobal() override = default;

public:
	void Initialize() override;
	void Release() override;

	void		SetCurrentFrameFence(const FenceHandle& handle) { m_currentFrameFence = handle; }
	FenceHandle GetCurrentFrameFence() const { return m_currentFrameFence; }

	void DeferDescriptorFree(
		DxDescriptorHeapGlobal* heap,
		uint32_t				descriptorIndex,
		const FenceHandle&		fenceHandle,
		std::string_view		debugName
	);
	void DeferResourceRelease(
		ComPtr<ID3D12Resource> resource, const FenceHandle& fenceHandle, std::string_view debugName = ""
	);
	void DeferRelease(
		std::function<void()> releaseCallback, const FenceHandle& fenceHandle, std::string_view debugName = ""
	);

	void ProcessCompleted(const CompletedFences& completedFences);
	void ProcessCompletedReleases(uint64_t completedFenceValue);

	void FlushAll();
	void LogStats() const;

private:
	struct ReleaseEntry
	{
		std::function<void()> releaseCallback;
		FenceHandle			  fenceHandle;
		// #ifdef _DEBUG
		std::string debugName;
		// #endif // _DEBUG
	};

	std::array<std::deque<ReleaseEntry>, 3> m_releaseQueue;
	uint32_t								m_totalProcessed = 0;
	FenceHandle								m_currentFrameFence;
};