#include "stdafxClientFramework.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxDescriptorHeapGlobal.h"
#include <string_view>

void DxGarbageCollectorGlobal::Initialize()
{
	m_currentFrameFence = FenceHandle();
	m_totalProcessed = 0;
	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Initialized\n");
}

void DxGarbageCollectorGlobal::Release()
{
	FlushAll();
	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Released: Total processed={}\n", m_totalProcessed);
}

void DxGarbageCollectorGlobal::DeferDescriptorFree(
	DxDescriptorHeapGlobal* heap, uint32_t descriptorIndex, const FenceHandle& fenceHandle, std::string_view debugName
)
{
	if (!heap)
	{
		GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] ERROR: Null heap\n");
		return;
	}

	auto callback = [heap, descriptorIndex]() { heap->FreeImmediate(descriptorIndex); };
	DeferRelease(callback, fenceHandle, debugName);
}

void DxGarbageCollectorGlobal::DeferResourceRelease(
	ComPtr<ID3D12Resource> resource, const FenceHandle& fenceHandle, std::string_view debugName
)
{
	if (!resource)
	{
		GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] ERROR: Null resource\n");
		return;
	}

	auto callback = [r = std::move(resource)]() mutable
	{
		if (r)
		{
			ComPtr<ID3D12Device> device;
			if (SUCCEEDED(r->GetDevice(IID_PPV_ARGS(&device))))
			{
				HRESULT hr = device->GetDeviceRemovedReason();
				if (FAILED(hr))
				{
					GRAPHICS_LOG_FMT(
						"[DxGarbageCollectorGlobal] WARNING: Device removed (HRESULT=0x{:X}), skipping Release()\n",
						static_cast<uint32_t>(hr)
					);
					r.Detach();
					return;
				}
			}
			r.Reset();
		}
	};
	DeferRelease(callback, fenceHandle, debugName);
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
						   m_releaseQueue[static_cast<int>(EQueueType::Copy)].size();

	GRAPHICS_LOG_FMT("[DxGarbageCollectorGlobal] Stats: Pending={}, TotalProcessed={}\n", pending, m_totalProcessed);
}
