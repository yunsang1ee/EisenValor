#pragma once
#include "Singleton.h"
#include <deque>
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

	void DeferDescriptorFreeAfterCurrentFrame(
		DxDescriptorHeapGlobal* heap, uint32_t descriptorIndex, std::string_view debugName
	);
	void DeferResourceRelease(
		ComPtr<ID3D12Resource> resource, const FenceHandle& fenceHandle, std::string_view debugName = ""
	);
	void DeferResourceReleaseAfterCurrentFrame(
		ComPtr<ID3D12Resource> resource, std::string_view debugName = ""
	);
	void DeferRelease(
		std::function<void()> releaseCallback, const FenceHandle& fenceHandle, std::string_view debugName = ""
	);
	void DeferReleaseAfterCurrentFrame(
		std::function<void()> releaseCallback, std::string_view debugName = ""
	);
	void CommitCurrentFrameReleases(const FenceHandle& frameFence);

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
	std::deque<ReleaseEntry>                 m_currentFrameReleaseQueue;
	uint32_t								m_totalProcessed = 0;
};
