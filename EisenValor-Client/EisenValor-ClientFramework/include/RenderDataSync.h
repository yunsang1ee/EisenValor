#pragma once
#include "DenseList.h"
#include "DxBuffer.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxUploadHeap.h"
#include "DxCommandContext.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxUtils.h"

template <typename T>
	requires std::is_trivially_copyable_v<T>
class RenderDataSync
{
public:
	using Handle = HandleOf<T>;

public:
	RenderDataSync() = default;
	~RenderDataSync() = default;

	RenderDataSync(const RenderDataSync&) = delete;
	RenderDataSync& operator=(const RenderDataSync&) = delete;

	RenderDataSync(RenderDataSync&&) noexcept = default;
	RenderDataSync& operator=(RenderDataSync&&) noexcept = default;

	[[nodiscard]] Handle Register(const T& data) { return m_list.Emplace(data); }
	[[nodiscard]] Handle Register(T&& data) { return m_list.Emplace(std::move(data)); }

	template <typename... Args>
		requires std::constructible_from<T, Args...> &&
				 (sizeof...(Args) != 1 || !std::same_as<std::decay_t<Args>..., T>)
	[[nodiscard]] Handle Register(Args&&... args)
	{
		return m_list.Emplace(std::forward<Args>(args)...);
	}

	void Unregister(Handle handle)
	{
		if (m_list.IsValid(handle))
			m_list.Remove(handle);
	}

	void Update(Handle handle, const T& data)
	{
		if (m_list.IsValid(handle))
		{
			m_list[handle] = data;
		}
	}

	[[nodiscard]] const T* TryGet(Handle handle) const { return m_list.TryGet(handle); }
	void				   Clear() { m_list.Reset(); }

	void SyncToGPU(ID3D12Device* device, DxCommandContext& context, DxUploadHeap& uploadHeap)
	{
		if (m_list.empty())
			return;

		size_t elementCount = m_list.size();
		size_t requiredBytes = elementCount * sizeof(T);

		// 버퍼 리사이징
		if (!m_gpuBuffer || m_currentCapacity < elementCount)
		{
			ResizeBuffer(device, elementCount);
		}

		UploadData(context, uploadHeap, requiredBytes);
	}

	[[nodiscard]] uint32_t GetSRVIndex() const { return m_gpuBuffer ? m_gpuBuffer->GetSRVHandle()->GetIndex() : ~0u; }

private:
	void ResizeBuffer(ID3D12Device* device, size_t requiredCount)
	{
		size_t newCapacity = std::max<size_t>(requiredCount * 3 / 2, 64);
		size_t sizeInBytes = newCapacity * sizeof(T);

		if (m_gpuBuffer)
		{
			m_gpuBuffer.reset();
		}

		m_gpuBuffer = std::make_unique<DxBuffer>();
		m_gpuBuffer->Initialize(
			device, sizeInBytes, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, L"RenderDataSync_Buffer"
		);

		m_gpuBuffer->CreateSRV(device, MANAGER(DxDescriptorHeapGlobal), (uint32_t)newCapacity, (uint32_t)sizeof(T));

		m_currentCapacity = newCapacity;

		DEBUG_LOG_FMT("[RenderDataSync] Resized GPU buffer to {} elements ({} bytes)", newCapacity, sizeInBytes);
	}

	void UploadData(DxCommandContext& context, DxUploadHeap& uploadHeap, size_t sizeInBytes)
	{
		auto* cmdList = context.CommandList();
		auto* destRes = m_gpuBuffer->GetResource();

		auto barrier1 =
			DxUtils::CreateTransitionBarrier(destRes, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdList->ResourceBarrier(1, &barrier1);

		auto alloc = uploadHeap.Allocate(sizeInBytes, 256);
		std::memcpy(alloc.cpuAddress, m_list.data(), sizeInBytes);

		cmdList->CopyBufferRegion(destRes, 0, uploadHeap.GetResource(), alloc.offset, sizeInBytes);

		auto barrier2 = DxUtils::CreateTransitionBarrier(
			destRes, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier2);

		DEBUG_LOG_FMT("[RenderDataSync] Uploaded {} elements ({} bytes) to GPU\n", m_list.size(), sizeInBytes);
	}

private:
	DenseList<T>			  m_list;
	std::unique_ptr<DxBuffer> m_gpuBuffer;
	size_t					  m_currentCapacity = 0;
};