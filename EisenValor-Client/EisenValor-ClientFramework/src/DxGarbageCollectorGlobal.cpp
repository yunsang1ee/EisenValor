#include "stdafxClientFramework.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxDescriptorHeapGlobal.h"
#include <string_view>

std::function<void()> DxGarbageCollectorGlobal::MakeResourceReleaseCallback(
	ComPtr<ID3D12Resource> resource, std::function<void()> onFinalized
)
{
	return [resource = std::move(resource), onFinalized = std::move(onFinalized)]() mutable
	{
		if (!resource)
		{
			if (onFinalized)
			{
				onFinalized();
			}
			return;
		}

		ComPtr<ID3D12Device> device;
		if (SUCCEEDED(resource->GetDevice(IID_PPV_ARGS(&device))))
		{
			const HRESULT hr = device->GetDeviceRemovedReason();
			if (FAILED(hr))
			{
				GRAPHICS_LOG_FMT(
					"[DxGarbageCollectorGlobal] WARNING: Device removed (HRESULT=0x{:X}), skipping Release()\n",
					static_cast<uint32_t>(hr)
				);
				resource.Detach();
				if (onFinalized)
				{
					onFinalized();
				}
				return;
			}
		}
		resource.Reset();

		if (onFinalized)
		{
			onFinalized();
		}
	};
}

void DxGarbageCollectorGlobal::Initialize()
{
	m_totalProcessed = 0;
	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Initialized\n");
}

void DxGarbageCollectorGlobal::Release()
{
	FlushAll();
	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Released: Total processed={}\n", m_totalProcessed);
}

void DxGarbageCollectorGlobal::DeferDescriptorFreeAfterCurrentFrame(
	DxDescriptorHeapGlobal* heap, uint32_t descriptorIndex, std::string_view debugName
)
{
	if (!heap)
	{
		GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] ERROR: Null heap\n");
		return;
	}

	auto callback = [heap, descriptorIndex]() { heap->FreeImmediate(descriptorIndex); };
	DeferReleaseAfterCurrentFrame(callback, debugName);
}

void DxGarbageCollectorGlobal::DeferResourceRelease(
	ComPtr<ID3D12Resource> resource,
	const FenceHandle&	   fenceHandle,
	std::string_view	   debugName,
	std::function<void()>  onFinalized
)
{
	if (!resource)
	{
		GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] ERROR: Null resource\n");
		return;
	}

	DeferRelease(MakeResourceReleaseCallback(std::move(resource), std::move(onFinalized)), fenceHandle, debugName);
}

void DxGarbageCollectorGlobal::DeferResourceReleaseAfterCurrentFrame(
	ComPtr<ID3D12Resource> resource, std::string_view debugName, std::function<void()> onFinalized
)
{
	if (!resource)
	{
		GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] ERROR: Null resource\n");
		return;
	}

	DeferReleaseAfterCurrentFrame(MakeResourceReleaseCallback(std::move(resource), std::move(onFinalized)), debugName);
}

void DxGarbageCollectorGlobal::DeferRelease(
	std::function<void()> releaseCallback, const FenceHandle& fenceHandle, std::string_view debugName
)
{
	const auto qi = static_cast<int>(fenceHandle.queueType);
#ifdef _DEBUG
	if (!m_releaseQueue[qi].empty())
	{
		const auto& back = m_releaseQueue[qi].back().fenceHandle;
		if (back.value > fenceHandle.value)
		{
			GRAPHICS_LOG_FMT(
				"[DxGarbageCollectorGlobal] ASSERT: non-monotonic fence value (prev={}, cur={})\n", back.value,
				fenceHandle.value
			);
			assert(false && "Fence value must be monotonic per fence.");
		}
	}
#endif // _DEBUG
	m_releaseQueue[qi].push_back(ReleaseEntry{std::move(releaseCallback), fenceHandle, std::string(debugName)});
	GRAPHICS_LOG_FMT(
		"[DxGarbageCollectorGlobal] Deferred release: Q={}, Val={}, Name={}\n", static_cast<int>(fenceHandle.queueType),
		fenceHandle.value, debugName
	);
}

void DxGarbageCollectorGlobal::DeferReleaseAfterCurrentFrame(
	std::function<void()> releaseCallback, std::string_view debugName
)
{
	m_currentFrameReleaseQueue.push_back(
		ReleaseEntry{std::move(releaseCallback), FenceHandle{}, std::string(debugName)}
	);
	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Deferred release until current frame completes: Name={}\n", debugName);
}

void DxGarbageCollectorGlobal::CommitCurrentFrameReleases(const FenceHandle& frameFence)
{
	while (!m_currentFrameReleaseQueue.empty())
	{
		auto entry = std::move(m_currentFrameReleaseQueue.front());
		m_currentFrameReleaseQueue.pop_front();
		DeferRelease(std::move(entry.releaseCallback), frameFence, entry.debugName);
	}
}

void DxGarbageCollectorGlobal::ProcessCompleted(const CompletedFences& completedFences)
{
	auto consume = [this](std::deque<ReleaseEntry>& q, uint64_t snapshotDone)
	{
		while (!q.empty())
		{
			const auto& e = q.front();

			if (snapshotDone >= e.fenceHandle.value)
			{
				auto fn = std::move(q.front().releaseCallback);
				auto debugName = std::move(q.front().debugName);
				q.pop_front();
				try
				{
					fn();
				}
				catch (const std::exception& ex)
				{
#if ENABLE_GRAPHICS_DEBUG_LOG
					GRAPHICS_LOG_FMT(
						"[DxGarbageCollectorGlobal] ERROR: Release failed for '{}': {}\n", debugName, ex.what()
					);
#else
					(void)ex;
#endif
				}
				++m_totalProcessed;
			}
			else
			{
				break;
			}
		}
	};

	consume(m_releaseQueue[static_cast<int>(EQueueType::Graphics)], completedFences.graphics);
	consume(m_releaseQueue[static_cast<int>(EQueueType::Compute)], completedFences.compute);
	consume(m_releaseQueue[static_cast<int>(EQueueType::Copy)], completedFences.copy);
}

void DxGarbageCollectorGlobal::ProcessCompletedReleases(uint64_t completedFenceValue)
{
	CompletedFences snap{};
	snap.graphics = completedFenceValue;
	ProcessCompleted(snap);
}

void DxGarbageCollectorGlobal::FlushAll()
{
	while (!m_currentFrameReleaseQueue.empty())
	{
		auto fn = std::move(m_currentFrameReleaseQueue.front().releaseCallback);
		auto debugName = std::move(m_currentFrameReleaseQueue.front().debugName);
		m_currentFrameReleaseQueue.pop_front();
		try
		{
			fn();
		}
		catch (const std::exception& ex)
		{
#if ENABLE_GRAPHICS_DEBUG_LOG
			GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] ERROR: Release failed for '{}': {}\n", debugName, ex.what());
#else
			(void)ex;
#endif
		}
		++m_totalProcessed;
	}

	for (int qi = 0; qi < 3; ++qi)
	{
		auto& q = m_releaseQueue[qi];
		while (!q.empty())
		{
			auto fn = std::move(q.front().releaseCallback);
			auto debugName = std::move(q.front().debugName);
			q.pop_front();
			try
			{
				fn();
			}
			catch (const std::exception& ex)
			{
#if ENABLE_GRAPHICS_DEBUG_LOG
				GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] ERROR: Release failed for '{}': {}\n", debugName, ex.what());
#else
				(void)ex;
#endif
			}
			++m_totalProcessed;
		}
	}
	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Flushed. TotalProcessed={}\n", m_totalProcessed);
}

void DxGarbageCollectorGlobal::LogStats() const
{
	const size_t pending = m_releaseQueue[static_cast<int>(EQueueType::Graphics)].size() +
						   m_releaseQueue[static_cast<int>(EQueueType::Compute)].size() +
						   m_releaseQueue[static_cast<int>(EQueueType::Copy)].size() +
						   m_currentFrameReleaseQueue.size();

	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Stats: Pending={}, TotalProcessed={}\n", pending, m_totalProcessed);
}
